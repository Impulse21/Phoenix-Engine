#include "HordeRenderer.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/Renderer/ShaderCompiler.h>

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
    constexpr u64 k_texture_heap_size = 16_MB;

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

    m_hdr_render_targets.Initialize(m_texture_allocator);

    // -- Create PSOs ---
    rhi::DeviceCapabilities cap = rhi::GetDeviceCapabilities();
    PHX_ASSERT(EnumHasAnyFlags(cap.features, rhi::DeviceFeatures::MeshShaders));

    rhi::ViewportDesc present_desc;
    if (!rhi::GetViewportDesc(present_desc))
    {
        PHX_LOG_ERROR(k_log, "Initialize failed — RHI has no viewport yet");
        return false;
    }

    ShaderCompiler::Initialize();

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
        },
        present_desc);

        m_pso[Pso::Capsule] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Capsule" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        },
        present_desc);

        m_pso[Pso::Plane] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Plane" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        },
        present_desc);

        rhi::DestroyShaderModule(primitive_shader_module);
    }

    ShaderCompiler::Shutdown();

    return true;
}

void horde::HordeRenderer::Shutdown()
{
    rhi::DeferUntilGpuComplete([this] {
        m_hdr_render_targets.DestroyFrameRenderTargets();
        rhi::DestroyGpuHeap(m_rebar_heap);
        rhi::DestroyTextureHeap(m_texture_heap);
    });

    for (rhi::PipelineStateHandle& pso : m_pso)
        rhi::DestroyPipelineState(pso);

}

void horde::HordeRenderer::PreRender(const phx::ecs::World& world, phx::FrameAllocator& frame_allocator, const hlslpp::float4x4& view_proj)
{
    m_curr_render_list = frame_allocator.Alloc<RenderList>();
    m_curr_render_list->num_render_packets = 0;
    m_curr_render_list->render_packets = frame_allocator.Alloc<RenderPacket>(kPsoCount);

    rhi::GpuBumpAllocator& gpu_frame_allocator = GetFrameGpuAllocaor();
    gpu_frame_allocator.Reset();

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

phx::rhi::PipelineStateHandle horde::HordeRenderer::CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_stages, const phx::rhi::ViewportDesc& viewport_desc) const
{
    rhi::Format colour_format = viewport_desc.format;
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
                .color_attachments = Span<rhi::Format>(&colour_format, 1),
                .depth_stencil_format = viewport_desc.depth_format,
            },
        });
}
