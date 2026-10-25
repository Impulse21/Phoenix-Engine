#include "HordeRenderer.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Result.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Renderer/RenderGraph.h>

#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/Renderer/ToneMapBlit.h>
#include <PhxEngine/Renderer/StandardSamplers.h>

#include "Shaders/primitives_interop.h"

#include "FrameData.h"
#include "WorldComponents.h"

#include <stb_image.h>

using namespace horde;
using namespace phx;
using namespace phx::rhi;


PHX_CVAR_FLOAT(renderer_exposure, -1.0f, "Exposure for tone mapping");

namespace
{
    constexpr Log::Channel k_log = { "Hord Renderer" };

    constexpr u64 k_rebar_heap_size = 20_MB;
    constexpr u64 k_per_frame_size = 1_MB;

    /*
        colour: 1920×1080×8 ≈ 15.82 MB
        depth: 1920×1080×4 ≈ 7.91 MB
        × 2 frames in flight ≈ 47.5 MB total

        TODO: Would be cool to request this info from the HDR render targets to help get an understanding of heap size.
    */
    constexpr u64 k_texture_heap_size = 100_MB;
    constexpr u32 kMaxTextureDescriptors = 256;

    hlslpp::interop::float3 ColourFor(const phx::ecs::World& world, phx::ecs::EntityId e)
    {
        if (world.TryGet<PlayerTagComponent>(e))
            return hlslpp::float3(0.2f, 1.0f, 0.2f);

        if (world.TryGet<EnemyTagComponent>(e))
            return hlslpp::float3(1.0f, 0.2f, 0.2f);

        return hlslpp::float3(0.6f, 0.6f, 0.6f);
    }
}

bool horde::HordeRenderer::Initialize() noexcept
{
    // -- Create allocations required for rendering ---
    PHX_LOG_INFO(k_log, "Allocating rebar heap size {0} MB", PhxBytesToMB(k_rebar_heap_size));
    m_rebar_heap = rhi::AllocateGpuHeap(k_rebar_heap_size, GpuMemoryType::CpuVisible);

    // -- Carve up the heap for upload and temp frame data

    GpuBumpAllocator upload_allocator;

    // -- Carve out rebar space ---
    {
        GpuBumpAllocator rebar_allocator;
        rebar_allocator.Initialize(m_rebar_heap.range);

        PHX_LOG_INFO(k_log, "Carving {0} MB per frame", PhxBytesToMB(k_per_frame_size));
        for(usize i = 0; i < m_frame_allocators.Size(); ++i)
        {
            GpuCpuRange<byte> frame_range = rebar_allocator.Alloc(k_per_frame_size);
            m_frame_allocators[i].Initialize(frame_range);
        }
        
        // Take a slice of the memory after the above check for upload space
        // TOOD: Query optimimal copy alignement
        const usize upload_size = rebar_allocator.Capacity() - rebar_allocator.Used();

        GpuCpuRange<byte> upload_range = rebar_allocator.Alloc(upload_size);
        upload_allocator.Initialize(upload_range);
    }

    // Set up Texture heap
    PHX_LOG_INFO(k_log, "Allocating texture heap size {0} MB", PhxBytesToMB(k_texture_heap_size));
    m_texture_heap = rhi::AllocateTextureHeap(k_texture_heap_size);
    m_texture_allocator.Initialize(m_texture_heap);

    rhi::RenderDeviceCapabilities cap = rhi::GetRenderDeviceCapabilities();
    PHX_LOG_INFO(
        Log::Channels::App,
        "Allocating Descriptor Heap {0} MB and Sampler Heap {1} MB",
        cap.image_descriptor_size,
        cap.sampler_descriptor_size);

    m_texture_descriptor_heap = rhi::AllocateGpuHeap(kMaxTextureDescriptors * cap.image_descriptor_size, rhi::GpuMemoryType::TextureDescriptorHeap);
    m_tex_descriptor_alloc.Initialize(m_texture_descriptor_heap.range, cap.image_descriptor_size);
    m_sampler_descriptor_heap = rhi::AllocateGpuHeap(renderer::kStandardSamplerCount * cap.sampler_descriptor_size, rhi::GpuMemoryType::SamplerDescriptorHeap);

    m_texture_upload_ticket = PreloadedTextures(upload_allocator, m_texture_allocator, m_tex_descriptor_alloc);
    phx::renderer::WriteStandardSamplers(m_sampler_descriptor_heap.range, cap.sampler_descriptor_size);

    ShaderCompiler::Initialize({
        .shader_search_paths = { 
            "shaders://", 
            "engine_shaders://" ,
            "engine_shaders://Shared" 
        },
    });


    // -- Render graph set up ---
    // m_rg_registry.Initialize();

    m_hdr_render_targets.Initialize(m_texture_allocator, m_tex_descriptor_alloc);


    // -- Older PSO set up --
    // -- Create PSOs ---
    PHX_ASSERT(EnumHasAnyFlags(cap.features, rhi::DeviceFeatures::MeshShaders));

    if (!ToneMapBlit::Initialize())
        return false;

    {
        auto primitive_module_spirv = ShaderCompiler::CompileModule("shaders://primitives.slang");
        if (!primitive_module_spirv)
        {
            PHX_LOG_ERROR(k_log, "Failed to compile primitive shader module");
            ShaderCompiler::Shutdown();
            return false;
        }

        phx::Result<phx::MemoryBuffer> forward_pass_spirv = 
            ShaderCompiler::Compile(
                "shaders://primitives_forward_pass.slang",
                "FS_Main",
                ShaderCompiler::Stage::Fragment,
                { .type_args = {
                    {.module_name = "MaterialInterface", .type_name = "Standard"}}
                }
            );

        if (!forward_pass_spirv)
        {
            PHX_LOG_ERROR(k_log, "Failed to compile primitive foward pass shader");
            ShaderCompiler::Shutdown();
            return false;
        }

        rhi::ShaderModuleHandle primitive_shader_module = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(primitive_module_spirv->Data()),
                primitive_module_spirv->Size() / sizeof(u32)),
        });

        rhi::ShaderModuleHandle forward_pass_module = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(forward_pass_spirv->Data()),
                forward_pass_spirv->Size() / sizeof(u32)),
        });

        m_pso[Pso::Box] =
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Box" },
                { .stage = rhi::ShaderStage::FS, .module_handle = forward_pass_module, .entry_point = "FS_Main" }
        });

        m_pso[Pso::Capsule] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Capsule" },
                { .stage = rhi::ShaderStage::FS, .module_handle = forward_pass_module, .entry_point = "FS_Main" }
        });

        m_pso[Pso::Plane] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Plane" },
                { .stage = rhi::ShaderStage::FS, .module_handle = forward_pass_module, .entry_point = "FS_Main" }
        });

        rhi::DestroyShaderModule(primitive_shader_module);
        rhi::DestroyShaderModule(forward_pass_module);
    }

    ShaderCompiler::Shutdown();

    m_is_textures_loaded = rhi::IsTicketFinished(m_texture_upload_ticket);
    return true;
}

void horde::HordeRenderer::Shutdown()
{
    ToneMapBlit::Shutdown();

    for (Texture& texture : m_preloaded_textures)
    {
        m_texture_allocator.Free(texture.placed_texture);
        texture = {};
    }

    m_texture_allocator.Shutdown();

    rhi::DeferUntilGpuComplete([this] {
        // m_rg_registry.Shutdown();
        m_hdr_render_targets.DestroyFrameRenderTargets();
        rhi::DestroyGpuHeap(m_rebar_heap);
        rhi::DestroyGpuHeap(m_sampler_descriptor_heap);
        rhi::DestroyGpuHeap(m_texture_descriptor_heap);
        rhi::DestroyTextureHeap(m_texture_heap);
    });

    for (rhi::PipelineStateHandle& pso : m_pso)
        rhi::DestroyPipelineState(pso);

}

void horde::HordeRenderer::PreRender(
    const phx::ecs::World& world,
    phx::ecs::EntityId main_camera_id,
    FrameData& frame_data)
{

    rhi::GpuBumpAllocator& gpu_frame_allocator = GetFrameGpuAllocaor();
    gpu_frame_allocator.Reset();

    GpuCpuRange<shader_interop::FrameConstants> gpu_frame_constants =
        gpu_frame_allocator.Alloc<shader_interop::FrameConstants>();

    if (!m_is_textures_loaded)
        m_is_textures_loaded = rhi::IsTicketFinished(m_texture_upload_ticket);

    // TODO: BUILD MATERIAL LIST.
    // Hard coded for now
    GpuCpuRange<shader_interop::Material> gpu_mtls = gpu_frame_allocator.Alloc<shader_interop::Material>(m_preloaded_textures.size());
    for (u32 i = 0; i < m_preloaded_textures.size(); ++i)
    {
        gpu_mtls.cpu[i] = {
            .albedo = m_preloaded_textures[i].descriptor,
            .sampler = phx::renderer::ToDescriptorIndex(phx::renderer::StandardSampler::LinearWrap),
            .roughness = 0.8f,
            .metalness = 0.1f,
        };
    }
    // -- Plane ---
    BuildViews(world, main_camera_id, gpu_frame_allocator, gpu_frame_constants, frame_data);

    #if false
    // -- Do any clean up on resources ---
    // TODO Build world

    m_rg_registry.BeginFrame();

    // Reset frame cached data;
    m_cached_frame_data = {};
    // m_cached_frame_data.views = BuildRenderViews(world, frame_allocator);

    phx::renderer::RenderGraph::Builder rg_builder(m_rg_registry);

    struct GeometryPassData
    {
        u32 num_render_packets;
        phx::FramePtr<RenderPacket> render_packets;
    };
    
    // TODO: Fill in render grah data.
    rg_builder.AddPass<GeometryPassData>(
        "forward_lighting_pass",
        [&](phx::renderer::PassBuilder& pass_builder)
        {
            // TODO: Set max so I can avoid this double count
            u32 view_count = 0;
            for(auto& view : m_cached_frame_data.views)
            {
                if(view.kind == RenderViewKind::Camera)
                    view_count++;
            }

            auto pass_views = 
                frame_allocator.AllocSpan<GeometryPassData>(view_count);

            u32 view_count = 0;
            for(auto& view : m_cached_frame_data.views)
            {
                if(view.kind == RenderViewKind::Camera)
                {
                    // Build Geometry pass data
                    GeometryPassData& pass = pass_views[view_count++];
                }
            }

            pass_builder.SetCachedData(ToVoidSpan(pass_views));
        },
        [](const GeometryPassData& cached_data, rhi::CommandBuffer cmd)
        {
            // TODO: Render
        });

    m_cached_frame_data.rg_compiled = rg_builder.Compile();

    // Build the Render Graph
    m_curr_render_list = frame_allocator.Alloc<RenderList>();
    m_curr_render_list->num_render_packets = 0;
    m_curr_render_list->render_packets = frame_allocator.Alloc<RenderPacket>(kPsoCount);

    rhi::GpuBumpAllocator& gpu_frame_allocator = GetFrameGpuAllocaor();
    gpu_frame_allocator.Reset();

    hlslpp::float4x4 view_proj = hlslpp::float4x4::identity();
    hlslpp::float3 camera_pos = hlslpp::float3(1.0f);

    bool found_camera = false;

    world.Each<CameraComponent, TransformComponent>(
        [&](ecs::EntityId, const CameraComponent& camera, const TransformComponent& transform)
    {
        if (found_camera)
            return;

        found_camera = true;

        // TODO: Move to a core utility function so I don't  have to remember this every time.
       
    });

    if (!found_camera)
        PHX_LOG_WARN(k_log, "No CameraComponent found in the world -- rendering with an identity view_proj");

    // Frame Data
    GpuCpuRange<shader_interop::FrameData> frame_data =
        gpu_frame_allocator.Alloc<shader_interop::FrameData>();

    frame_data->view_proj = view_proj;
    frame_data->camera_pos = camera_pos;

#endif
}

void horde::HordeRenderer::Render(const FrameData& frame_data)
{
    // TODO: Implement render graph logic

    rhi::GpuBumpAllocator& gpu_frame_allocator = GetFrameGpuAllocaor();

    // TODO: Loop through views

    // TODO: Cache this?
    rhi::ViewportDesc viewport_desc;
    rhi::GetViewportDesc(viewport_desc);

    // TODO - Cache this?
    Span<const renderer::FrameRenderTargets> render_targets = m_hdr_render_targets.GetOrCreateFrameRenderTargets(viewport_desc.width, viewport_desc.height);
    const renderer::FrameRenderTargets& curr_targets = render_targets[rhi::GetFrameIndex()];

    // -- Begin Renderer ---
    // -- Forward Lighting Pass ---
    phx::rhi::CommandBuffer cmd = phx::rhi::BeginCommandRecording(phx::rhi::CommandQueueType::Graphics);

    rhi::CmdSetDescriptorHeaps(cmd, m_texture_descriptor_heap, m_sampler_descriptor_heap);
    rhi::CmdBeginRenderPass(
        cmd,
        curr_targets.scene_colour,
        { .colour = { 0.0f, 0.0f, 0.0f, 1.0f}},
        curr_targets.depth,
        { .depth_stencil = { .depth = 1.0f } });

    // Only consider using the main camera for now
    // In the future, we will need to pass the view pointer to a pass
    // in the graph to then record the data. This will handle the 
    // Begin Passes naturally.
    PHX_ASSERT(frame_data.views.size() > 1);
    const RenderView& main_view = frame_data.views[0];

    for (auto& render_packet : main_view.render_list)
    {
        rhi::CmdBindPipelineState(cmd, render_packet.pso_handle);
        rhi::CmdDispatchMesh(cmd, render_packet.draw_root.gpu, render_packet.instance_count, 1, 1);
    }

    phx::rhi::CmdEndRenderPass(cmd);

    rhi::CmdBarrier(cmd,
        rhi::BarrierStage::ColorOutput, rhi::BarrierAccess::ColorWrite,
        rhi::BarrierStage::Fragment, rhi::BarrierAccess::ShaderRead);
        
    ToneMapBlit::Blit(
        curr_targets.scene_colour_index,
        {
            .cmd_buffer = cmd,
            .gpu_frame_allocator = &gpu_frame_allocator,
        },
        CVar_renderer_exposure.Get());

    rhi::SubmitAndPresent(Span<rhi::CommandBuffer>(&cmd, 1));
}

phx::rhi::PipelineStateHandle horde::HordeRenderer::CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_stages) const
{
    return rhi::CreatePipelineState({
            .type           = rhi::PipelineType::Graphics,
            .shader_stages  = shader_stages,
            .depth_stencil_state = {
                .depth_enable     = true,
                .depth_write_mask = rhi::DepthWriteMask::All,
                .depth_func       = rhi::ComparisonFunc::Less, // matches the depth_clear = 1.0f (far) convention used in OnRender
            },
            .raster_state = {
                .cull_mode = rhi::RasterCullMode::Back,
                .front_counter_clockwise = !rhi::IsClipSpaceYDown(),
            },
            .prim_type      = rhi::PrimitiveType::TriangleList,
            .render_pass_info = {
                .color_attachments = Span<rhi::Format>(&renderer::HdrRenderTargets::k_colour_buffer_format, 1),
                .depth_stencil_format = renderer::HdrRenderTargets::k_depth_buffer_format,
            },
        });
}

void horde::HordeRenderer::BuildViews(
    const phx::ecs::World& world,
    phx::ecs::EntityId main_camera_id,
    phx::rhi::GpuBumpAllocator& frame_gpu_allocator,
    GpuCpuRange<shader_interop::FrameConstants>& gpu_frame_constants,
    FrameData& frame_data) const
{
    // -- Process Main Camera ---
    FrameList<RenderView> camera_render_views(*frame_data.allocator, 1);
    {
        auto* camera = world.TryGet<CameraComponent>(main_camera_id);
        auto* transform = world.TryGet<TransformComponent>(main_camera_id);
        PHX_ASSERT(camera && transform);

        rhi::ViewportDesc viewport_desc;
        rhi::GetViewportDesc(viewport_desc);

        RenderView render_view = {
            .Kind = RenderViewKind::Camera,
        };

        // -- Calculate aspect Ration --
        const float aspect =
            static_cast<float>(viewport_desc.width) / static_cast<float>(viewport_desc.height);

        render_view.view = hlslpp::float4x4::look_at(transform->position, camera->target, camera->up);

        // Calculate frustum
        const hlslpp::frustum frustum =
            hlslpp::frustum::field_of_view_y(hlslpp::radians(hlslpp::float1(camera->fov_y_degrees)),
                aspect,
                camera->near_plane,
                camera->far_plane);

        const hlslpp::projection proj_params(
            frustum, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);

        render_view.proj = hlslpp::float4x4::perspective(proj_params);
        if (rhi::IsClipSpaceYDown())
            render_view.proj = hlslpp::mul(render_view.proj, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));

        render_view.view_proj = hlslpp::mul(render_view.view, render_view.proj);

        u32 view_index = camera_render_views.GetSize();
        render_view.render_list = BuildRenderList(world, frame_gpu_allocator, frame_data, gpu_frame_constants, view_index);

        camera_render_views.Push(std::move(render_view));
    }

    frame_data.views = camera_render_views.ToSpan();

    {
        // Construct GPU view data
        rhi::GpuCpuRange<shader_interop::RenderView> gpu_render_views =
            frame_gpu_allocator.Alloc<shader_interop::RenderView>(frame_data.views.size());

        for (usize i = 0; i < frame_data.views.size(); ++i)
        {
            // Be mind full of the Access pattern of the GPU data
            const RenderView& render_view = frame_data.views[i];

            gpu_render_views.cpu[i] = {
                .view = render_view.view,
                .proj = render_view.proj,
                .view_proj = render_view.view_proj
            };
        }

        gpu_frame_constants->views = gpu_render_views.gpu;
    }
    // Set the pointer on the frame constants instance

    // -- TODO Lights ---
#if false
    FrameList<phx::renderer::VisibileLight> visibile_lights(*frame_data.allocator, 1);
    world.Each<LightComponent, TransformComponent>(
        [&](ecs::EntityId e, const LightComponent& light, const TransformComponent& transform)
    {
        if (!light.casts_shadows)
            return;

        PHX_ASSERT(light.type == LightType::Directional);   // only directional for now

        visibile_lights.Push(renderer::VisibileLight{
            .light_index = 0,
            .first_view = 0,
            .view_count = 1
        });
        const float half_extent = light.shadow_distance;
        const u32   resolution  = light.shadow_resolution;
        if (half_extent <= 0.0f || resolution == 0)
            return;

        const hlslpp::float3 light_dir =
            hlslpp::mul(transform.rotation, hlslpp::float3(0.0f, 0.0f, -1.0f));

        // Shadow camera sits half_extent behind the focus, looking at it
        const hlslpp::float3 light_position = shadow_focus - light_dir * half_extent;

        const hlslpp::float3 world_up(0.0f, 1.0f, 0.0f);
        const hlslpp::float3 up =
            (float(hlslpp::abs(hlslpp::dot(light_dir, world_up))) > 0.99f)   // verify float1 -> float
                ? hlslpp::float3(1.0f, 0.0f, 0.0f)
                : world_up;

        const hlslpp::float4x4 view =
            hlslpp::float4x4::look_at(light_position, shadow_focus, up);

        // Box is 2*half_extent square, depth 0 .. 2*half_extent, centered on the focus
        const float size = 2.0f * half_extent;
        const hlslpp::frustum box(size, size, 0.0f, size);                    // verify ctor: width, height, near, far
        const hlslpp::projection proj_params(
            box, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);

        hlslpp::float4x4 proj = hlslpp::float4x4::orthographic(proj_params);  // verify

        // Fold the flip into proj so proj and view_proj agree
        if (rhi::IsClipSpaceYDown())
            proj = hlslpp::mul(proj, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));

        const hlslpp::float4x4 view_proj = hlslpp::mul(view, proj);

        views.cpu[view_counter++] = phx::renderer::RenderView{
            .kind      = RenderViewKind::SunShadow,
            .proj      = proj,
            .view      = view,
            .view_proj = view_proj,
            .viewport  = { .x = 0, .y = 0, .width = resolution, .height = resolution },
        };
    });
#endif

}

Span<RenderPacket> horde::HordeRenderer::BuildRenderList(
    const phx::ecs::World& world,
    phx::rhi::GpuBumpAllocator& frame_gpu_allocator,
    const FrameData& frame_data,
    GpuCpuRange<shader_interop::FrameConstants> gpu_frame_constants,
    u32 view_index) const
{
    // TODO: Data Drive
    FrameList<RenderPacket> view_render_list(*frame_data.allocator, 100);

    // TODO: CULL
    
    // -- Process Primitives in the scene ---

    // -- Plane ---
    {
        const u32 num_planes = static_cast<u32>(world.Count<PlaneRenderComponent>());
        if (num_planes > 0)
        {
            GpuCpuRange<shader_interop::PlaneInstanceData> instances =
                frame_gpu_allocator.Alloc<shader_interop::PlaneInstanceData>(num_planes);

            u32 curr_index = 0;
            world.Each<PlaneRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const PlaneRenderComponent& plane, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_planes && "Count() didn't match what Each() visited");
                if (curr_index >= num_planes)
                    return;

                instances.cpu[curr_index++] = {
                    .model = ToMatrix(transform),
                    .colour =  ColourFor(world, e),
                    .extent = plane.extent,
                };
            });

            GpuCpuRange<shader_interop::PlaneDrawRoot> draw_root =
                frame_gpu_allocator.Alloc<shader_interop::PlaneDrawRoot>();

            *draw_root.cpu = {
                .base = { 
                    .frame_constants = gpu_frame_constants.gpu,
                    .mtl_index = static_cast<u32>(PreloadedTexture::BlockoutBlack) // hard coded for now.
                },
                .instances = instances.gpu,
                .view_index = view_index,
            };

            view_render_list.Push({
                .pso_handle     = m_pso[Pso::Plane],
                .draw_root      = draw_root.ToGpuRange(),
                .instance_count = curr_index,
            });
        }
    }
    
    // -- Capsules ---
    {
        const u32 num_capsules = static_cast<u32>(world.Count<CapsuleRenderComponent>());
        if (num_capsules > 0)
        {
            GpuCpuRange<shader_interop::CapsuleInstanceData> instances =
                frame_gpu_allocator.Alloc<shader_interop::CapsuleInstanceData>(num_capsules);

            u32 curr_index = 0;
            world.Each<CapsuleRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const CapsuleRenderComponent& capsule, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_capsules && "Count() didn't match what Each() visited");
                if (curr_index >= num_capsules)
                    return;
  
                instances.cpu[curr_index++] = {
                    .model = ToMatrix(transform),
                    .colour =  ColourFor(world, e),
                    .radius = capsule.radius,
                    .height = capsule.height,
                };
            });

            GpuCpuRange<shader_interop::CapsuleDrawRoot> draw_root =
                frame_gpu_allocator.Alloc<shader_interop::CapsuleDrawRoot>();

            *draw_root.cpu = {
                .base = { 
                    .frame_constants = gpu_frame_constants.gpu,
                    .mtl_index = rhi::kInvalidDescriptorIndex
                },
                .instances = instances.gpu,
                .view_index = view_index,
            };

            view_render_list.Push({
                .pso_handle     = m_pso[Pso::Capsule],
                .draw_root      = draw_root.ToGpuRange(),
                .instance_count = curr_index,
            });
        }
    }

    // -- Boxes ---
    {
        const u32 num_boxes = static_cast<u32>(world.Count<BoxRenderComponent>());
        if (num_boxes > 0)
        {
            GpuCpuRange<shader_interop::BoxInstanceData> instances =
                frame_gpu_allocator.Alloc<shader_interop::BoxInstanceData>(num_boxes);

            u32 curr_index = 0;
            world.Each<BoxRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const BoxRenderComponent& box, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_boxes && "Count() didn't match what Each() visited");
                if (curr_index >= num_boxes)
                    return;
  
                instances.cpu[curr_index++] = {
                    .model = ToMatrix(transform),
                    .colour =  ColourFor(world, e),
                    .extent = box.extent,
                };
            });

            GpuCpuRange<shader_interop::BoxDrawRoot> draw_root =
                frame_gpu_allocator.Alloc<shader_interop::BoxDrawRoot>();

            *draw_root.cpu = {
                .base = { 
                    .frame_constants = gpu_frame_constants.gpu,
                    .mtl_index = static_cast<u32>(PreloadedTexture::BlockoutGrey) // hard coded for now.
                },
                .instances = instances.gpu,
                .view_index = view_index,
            };

            view_render_list.Push({
                .pso_handle     = m_pso[Pso::Box],
                .draw_root      = draw_root.ToGpuRange(),
                .instance_count = curr_index,
            });
        }
    }

    return view_render_list.ToSpan();
}

phx::rhi::UploadTicket horde::HordeRenderer::PreloadedTextures(
    phx::rhi::GpuBumpAllocator& upload_allocator,
    phx::rhi::TextureAllocator& texture_allocator,
    phx::rhi::DescriptorAllocator& descriptor_allocator)
{
    using PreloadFileArray = std::array<std::pair<PreloadedTexture, const char*>, kPreloadTextureCount>;
    static constexpr PreloadFileArray k_textures_to_load =
    {
        std::pair{ 
            PreloadedTexture::BlockoutBlack,
            "assets_core://blockout-textures/png/512/basic/black_check.png"
        },
        std::pair {
            PreloadedTexture::BlockoutGrey,
            "assets_core://blockout-textures/png/512/basic/grey_check.png"
        }
    };

    rhi::CommandBuffer upload_cmd = rhi::BeginCommandRecording(rhi::CommandQueueType::Copy);

    for (auto& [type, filename] : k_textures_to_load)
    {
        bool successful = LoadTexture(
            filename,
            upload_allocator,
            texture_allocator,
            descriptor_allocator,
            upload_cmd,
            m_preloaded_textures[type]);

        if (successful == false)
            PHX_LOG_ERROR(Log::Channels::App, "Failed to load texture {0}", filename);
    }
    
    return rhi::SubmitUpload(upload_cmd);
}

bool horde::HordeRenderer::LoadTexture(
    const char* filename, 
    GpuBumpAllocator& upload_allocator,
    TextureAllocator& texture_allocator,
    DescriptorAllocator& descriptor_allocator,
    CommandBuffer cmd,
    Texture& texture)
{
    PHX_LOG_INFO(Log::Channels::App, "Loading texture {0}", filename);

    MemoryBuffer       image_memory = VFS::ReadFile(filename);
    TypedView<stbi_uc> data_view    = image_memory.GetView<stbi_uc>();

    int            width, height, channels;
    unsigned char* data = 
        stbi_load_from_memory(
            data_view.Get(),
            static_cast<u32>(image_memory.Size()),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha);

    if (data == nullptr)
    {
        PHX_LOG_ERROR(Log::Channels::App, "Failed to load PNG file: {0}", stbi_failure_reason());
        return false;
    }

    // Write the data directly to the CPU visiable memory
    const size_t  byte_count = (size_t)width * (size_t)height * 4;
    PHX_LOG_INFO(
        Log::Channels::App,
        "Request {0} kbs from buffer allocator with {1}/{2} kb of space",
        PhxBytesToKB(byte_count),
        upload_allocator.Used(),
        PhxBytesToKB(upload_allocator.Capacity()));


    rhi::GpuCpuRange<byte> upload_alloc = upload_allocator.Alloc(byte_count);
    if (!upload_alloc.IsValid())
    {
        PHX_LOG_ERROR(
            Log::Channels::App,
            "Unable to allocate upload space for texture of size {0}",
            PhxBytesToKB(byte_count));
        
        return false;
    }

    // Copy to CPU memory. Would be nice to read directly to the GPU memory
    std::memcpy(upload_alloc.cpu, data, byte_count);

    texture.placed_texture = m_texture_allocator.Alloc({
        .format = rhi::Format::SRGBA8_UNORM,
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
    });

    texture.descriptor = descriptor_allocator.Allocate(texture.placed_texture.handle);

    rhi::CmdCopyMemoryToTexture(
        cmd,
        upload_alloc.ToGpuRange(),
        texture.placed_texture.handle,
        {
            .extent = { static_cast<u32>(width), static_cast<u32>(height), 1 },
        });

    return true;
}
