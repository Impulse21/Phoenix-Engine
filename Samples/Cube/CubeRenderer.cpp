#include "CubeRenderer.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/Renderer/ShaderCompiler.h>


using namespace phx;

namespace
{
    constexpr Log::Channel k_log = { "Cube Renderer" };
	constexpr u64 k_buffer_heap_size = 1_MB;
}

bool samples::CubeRenderer::Initialize()
{
    ShaderCompiler::Initialize();

    // -- Initialize Heaps --- 
    PHX_LOG_INFO(k_log, "Allocating Buffer heap size {0} MB", k_buffer_heap_size);
    m_buffer_heap = rhi::AllocateGpuHeap(k_buffer_heap_size, phx::rhi::GpuMemoryType::CpuVisible);
    m_buffer_allocator.Initialize(m_buffer_heap.range);

    rhi::DeviceCapabilities cap = rhi::GetDeviceCapabilities();
    PHX_LOG_INFO(
        Log::Channels::App,
        "Allocating Descriptor Heap {0} MB and Sampler Heap {1} MB",
        cap.image_descriptor_size,
        cap.sampler_descriptor_size);

    m_texture_descriptor_heap = rhi::AllocateGpuHeap(cap.image_descriptor_size, rhi::GpuMemoryType::TextureDescriptorHeap);
    m_sampler_descriptor_heap = rhi::AllocateGpuHeap(cap.sampler_descriptor_size, rhi::GpuMemoryType::SamplerDescriptorHeap);

    m_texture_heap = rhi::AllocateTextureHeap(16);

     // -- Create required Pipelines and data ---
    auto vs_result = ShaderCompiler::Compile("shaders://Cube.slang", "VS_Main", ShaderCompiler::Stage::Vertex);
    auto fs_result = ShaderCompiler::Compile("shaders://Cube.slang", "FS_Main", ShaderCompiler::Stage::Fragment);

    // -- Load shaders ---
    if (!vs_result || !fs_result)
    {
        PHX_LOG_ERROR(k_log, "Failed to compile Cube.slang");
        return false;
    }

     m_vertex_shader   = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(vs_result->Data()),
                vs_result->Size() / sizeof(u32)),
        });

    m_fragment_shader = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(fs_result->Data()),
                fs_result->Size() / sizeof(u32)),
        });

    
    rhi::ShaderStageInfo stages[] = {
        { .stage = rhi::ShaderStage::VS, .module_handle = m_vertex_shader,   .entry_point = "VS_Main" },
        { .stage = rhi::ShaderStage::PS, .module_handle = m_fragment_shader, .entry_point = "FS_Main" },
    };

    rhi::ViewportDesc present_desc;
    if (!rhi::GetViewportDesc(present_desc))
    {
        PHX_LOG_ERROR(k_log, "Initialize failed — RHI has no viewport yet");
        return false;
    }


    rhi::Format colour_format = present_desc.format;
    m_cube_pipeline = rhi::CreatePipelineState({
        .type           = rhi::PipelineType::Graphics,
        .shader_stages  = stages,
        .depth_stencil_state = {
            .depth_enable     = true,
            .depth_write_mask = rhi::DepthWriteMask::All,
            .depth_func       = rhi::ComparisonFunc::Less, // matches the depth_clear = 1.0f (far) convention used in OnRender
        },
        .raster_state = {
            .cull_mode = rhi::RasterCullMode::None,
            .front_counter_clockwise = !rhi::IsClipSpaceYDown(),
        },
        .prim_type      = rhi::PrimitiveType::TriangleList,
        .render_pass_info = {
            .color_attachments = Span<rhi::Format>(&colour_format, 1),
            .depth_stencil_format = present_desc.depth_format,
        },
    });

    rhi::SamplerDescriptor desc = {
            .address_u  = rhi::SamplerAddressMode::Clamp,
            .address_v  = rhi::SamplerAddressMode::Clamp
    };

    rhi::WriteSamplerDescriptor(desc, m_sampler_descriptor_heap.range.cpu);

    phx::ShaderCompiler::Shutdown();
    return true;
}

void samples::CubeRenderer::Shutdown()
{
    rhi::DeferUntilGpuComplete([this]{
        rhi::DestroyGpuHeap(m_buffer_heap);
        rhi::DestroyGpuHeap(m_sampler_descriptor_heap);
        rhi::DestroyGpuHeap(m_texture_descriptor_heap);
        rhi::DestroyTextureHeap(m_texture_heap);
    });

    rhi::DestroyPipelineState(m_cube_pipeline);
    rhi::DestroyShaderModule(m_vertex_shader);
    rhi::DestroyShaderModule(m_fragment_shader);
}

void samples::CubeRenderer::Render(rhi::CommandBuffer cmd)
{
    PHX_ASSERT(m_cached_render_packet);
    if (m_cached_render_packet == nullptr)
        return;
        
    // TODO Have the rendere preformt he cache.
    const DrawData draw_data = {
        .vertices = m_cached_render_packet->mesh->vertices.gpu,
        .mvp = m_cached_render_packet->mvp,
    };

    phx::rhi::BindPipelineState(m_cube_pipeline, cmd);
    
    phx::rhi::DrawIndex(
        cmd,
        draw_data,
        m_cached_render_packet->mesh->indices.ToGpuRange(),
        rhi::IndexFormat::Uint32,
        m_cached_render_packet->mesh->indices.size);

    m_cached_render_packet = nullptr;
}
