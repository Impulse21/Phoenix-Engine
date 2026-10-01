#include "HordeRenderer.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/Renderer/ToneMapBlit.h>
#include <PhxEngine/Renderer/StandardSamplers.h>

#include "Shaders/primitives_interop.h"
#include "WorldComponents.h"

using namespace horde;
using namespace phx;
using namespace phx::rhi;

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

    PHX_LOG_INFO(k_log, "Carving {0} MB per frame", PhxBytesToMB(k_per_frame_size));
    for(usize i = 0; i < m_frame_allocators.Size(); ++i)
    {
        GpuCpuRange<byte> frame_range = m_rebar_heap->Slice(i * k_per_frame_size, k_per_frame_size);
        m_frame_allocators[i].Initialize(frame_range);
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
    
    phx::renderer::WriteStandardSamplers(m_sampler_descriptor_heap.range, cap.sampler_descriptor_size);

    m_hdr_render_targets.Initialize(m_texture_allocator, m_tex_descriptor_alloc);

    // -- Create PSOs ---
    PHX_ASSERT(EnumHasAnyFlags(cap.features, rhi::DeviceFeatures::MeshShaders));

    ShaderCompiler::Initialize();

    if (!ToneMapBlit::Initialize())
        return false;

    {
        auto primitive_module_spriv = ShaderCompiler::CompileModule("shaders://primitives.slang");
        if (!primitive_module_spriv)
        {
            PHX_LOG_ERROR(k_log, "Failed to compile primitive codes");
            ShaderCompiler::Shutdown();
            return false;
        }

        rhi::ShaderModuleHandle primitive_shader_module = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(primitive_module_spriv->Data()),
                primitive_module_spriv->Size() / sizeof(u32)),
        });

        m_pso[Pso::Box] =
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Box" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        });

        m_pso[Pso::Capsule] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Capsule" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        });

        m_pso[Pso::Plane] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Plane" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        });

        rhi::DestroyShaderModule(primitive_shader_module);
    }

    ShaderCompiler::Shutdown();

    return true;
}

void horde::HordeRenderer::Shutdown()
{
    ToneMapBlit::Shutdown();
    rhi::DeferUntilGpuComplete([this] {
        m_hdr_render_targets.DestroyFrameRenderTargets();
        rhi::DestroyGpuHeap(m_rebar_heap);
        rhi::DestroyGpuHeap(m_sampler_descriptor_heap);
        rhi::DestroyGpuHeap(m_texture_descriptor_heap);
        rhi::DestroyTextureHeap(m_texture_heap);
    });

    for (rhi::PipelineStateHandle& pso : m_pso)
        rhi::DestroyPipelineState(pso);

}

void horde::HordeRenderer::PreRender(const phx::ecs::World& world, phx::FrameAllocator& frame_allocator)
{
    m_curr_render_list = frame_allocator.Alloc<RenderList>();
    m_curr_render_list->num_render_packets = 0;
    m_curr_render_list->render_packets = frame_allocator.Alloc<RenderPacket>(kPsoCount);

    rhi::GpuBumpAllocator& gpu_frame_allocator = GetFrameGpuAllocaor();
    gpu_frame_allocator.Reset();

    hlslpp::float4x4 view_proj = hlslpp::float4x4::identity();
    bool found_camera = false;

    world.Each<CameraComponent, TransformComponent>(
        [&](ecs::EntityId, const CameraComponent& camera, const TransformComponent& transform)
    {
        if (found_camera)
            return;

        found_camera = true;

        // TODO: Move to a core utility function so I don't  have to remember this every time.
        rhi::ViewportDesc viewport_desc;
        rhi::GetViewportDesc(viewport_desc);
        const float aspect = static_cast<float>(viewport_desc.width) / static_cast<float>(viewport_desc.height);

        const hlslpp::float4x4 view = hlslpp::float4x4::look_at(transform.position, camera.target, camera.up);

        const hlslpp::frustum frustum = hlslpp::frustum::field_of_view_y(
            hlslpp::radians(hlslpp::float1(camera.fov_y_degrees)), aspect, camera.near_plane, camera.far_plane);
        const hlslpp::projection proj_params(frustum, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);
        const hlslpp::float4x4 proj = hlslpp::float4x4::perspective(proj_params);

        view_proj = hlslpp::mul(view, proj);

        if (rhi::IsClipSpaceYDown())
            view_proj = hlslpp::mul(view_proj, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));
    });

    if (!found_camera)
        PHX_LOG_WARN(k_log, "No CameraComponent found in the world -- rendering with an identity view_proj");

    // -- Capsules ---
    {
        const u32 num_capsules = static_cast<u32>(world.Count<CapsuleRenderComponent>());
        if (num_capsules > 0)
        {
            GpuCpuRange<shader_interop::CapsuleInstanceData> instances =
                gpu_frame_allocator.Alloc<shader_interop::CapsuleInstanceData>(num_capsules);

            u32 curr_index = 0;
            world.Each<CapsuleRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const CapsuleRenderComponent& capsule, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_capsules && "Count() didn't match what Each() visited");
                if (curr_index >= num_capsules)
                    return;

                shader_interop::CapsuleInstanceData& instance = instances.cpu[curr_index++];
                instance.mvp    = hlslpp::mul(ToMatrix(transform), view_proj);
                instance.colour = ColourFor(world, e);
                instance.radius = capsule.radius;
                instance.height = capsule.height;
            });

            m_curr_render_list->render_packets[m_curr_render_list->num_render_packets++] = {
                .pso_handle     = m_pso[Pso::Capsule],
                .instance_ptr   = instances.ToGpuRange(),
                .instance_count = curr_index,
            };
        }
    }

    // -- Boxes ---
    {
        const u32 num_boxes = static_cast<u32>(world.Count<BoxRenderComponent>());
        if (num_boxes > 0)
        {
            GpuCpuRange<shader_interop::BoxInstanceData> instances =
                gpu_frame_allocator.Alloc<shader_interop::BoxInstanceData>(num_boxes);

            u32 curr_index = 0;
            world.Each<BoxRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const BoxRenderComponent& box, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_boxes && "Count() didn't match what Each() visited");
                if (curr_index >= num_boxes)
                    return;

                shader_interop::BoxInstanceData& instance = instances.cpu[curr_index++];
                instance.mvp    = hlslpp::mul(ToMatrix(transform), view_proj);
                instance.colour = ColourFor(world, e);
                instance.extent = box.extent;
            });

            m_curr_render_list->render_packets[m_curr_render_list->num_render_packets++] = {
                .pso_handle     = m_pso[Pso::Box],
                .instance_ptr   = instances.ToGpuRange(),
                .instance_count = curr_index,
            };
        }
    }

    // -- Plane ---
    {
        const u32 num_planes = static_cast<u32>(world.Count<PlaneRenderComponent>());
        if (num_planes > 0)
        {
            GpuCpuRange<shader_interop::PlaneInstanceData> instances =
                gpu_frame_allocator.Alloc<shader_interop::PlaneInstanceData>(num_planes);

            u32 curr_index = 0;
            world.Each<PlaneRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const PlaneRenderComponent& plane, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_planes && "Count() didn't match what Each() visited");
                if (curr_index >= num_planes)
                    return;

                shader_interop::PlaneInstanceData& instance = instances.cpu[curr_index++];
                instance.mvp    = hlslpp::mul(ToMatrix(transform), view_proj);
                instance.colour = ColourFor(world, e);
                instance.extent = plane.extent;
            });

            m_curr_render_list->render_packets[m_curr_render_list->num_render_packets++] = {
                .pso_handle     = m_pso[Pso::Plane],
                .instance_ptr   = instances.ToGpuRange(),
                .instance_count = curr_index,
            };
        }
    }
}

void horde::HordeRenderer::Render()
{
    PHX_ASSERT(m_curr_render_list != nullptr);

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

    // TODO: Render Draw list
    for (u32 i = 0; i < m_curr_render_list->num_render_packets; ++i)
    {
        const RenderPacket& packet = m_curr_render_list->render_packets[i];

        rhi::CmdBindPipelineState(cmd, packet.pso_handle);
        rhi::CmdDispatchMesh(cmd, packet.instance_ptr.gpu, packet.instance_count, 1, 1);
    }

    phx::rhi::CmdEndRenderPass(cmd);

    rhi::CmdBarrier(cmd,
        rhi::BarrierStage::ColorOutput, rhi::BarrierAccess::ColorWrite,
        rhi::BarrierStage::Fragment, rhi::BarrierAccess::ShaderRead);

    ToneMapBlit::Blit(curr_targets.scene_colour_index, cmd);

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
