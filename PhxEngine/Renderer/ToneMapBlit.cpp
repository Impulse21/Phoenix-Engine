#include "ToneMapBlit.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/RHI/RHI.h>

using namespace phx;

namespace
{
    constexpr Log::Channel k_log = { "ToneMapBlit" };

    rhi::ShaderModuleHandle   s_vertex_shader;
    rhi::ShaderModuleHandle   s_fragment_shader;
    rhi::PipelineStateHandle  s_pipeline;

    // Mirrors ToneMapBlit.slang's PushConstants — keep these in sync.
    struct PushConstants
    {
        u32   scene_colour_index;
        float exposure;
    };

    rhi::ShaderModuleHandle CreateShaderModuleFromSpirv(const MemoryBuffer& spirv)
    {
        return rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(spirv.Data()),
                spirv.Size() / sizeof(u32)),
        });
    }
}

bool phx::ToneMapBlit::Initialize()
{
    rhi::ViewportDesc present_desc;
    if (!rhi::GetViewportDesc(present_desc))
    {
        PHX_LOG_ERROR(k_log, "Initialize failed — RHI has no viewport yet");
        return false;
    }

    auto vs_result = ShaderCompiler::Compile("engine_shaders://ToneMapBlit.slang", "VS_Main", ShaderCompiler::Stage::Vertex);
    auto fs_result = ShaderCompiler::Compile("engine_shaders://ToneMapBlit.slang", "FS_Main", ShaderCompiler::Stage::Fragment);

    if (!vs_result || !fs_result)
    {
        PHX_LOG_ERROR(k_log, "Failed to compile ToneMapBlit.slang");
        return false;
    }

    s_vertex_shader   = CreateShaderModuleFromSpirv(vs_result.GetValue());
    s_fragment_shader = CreateShaderModuleFromSpirv(fs_result.GetValue());

    rhi::ShaderStageInfo stages[] = {
        { .stage = rhi::ShaderStage::VS, .module_handle = s_vertex_shader,   .entry_point = "VS_Main" },
        { .stage = rhi::ShaderStage::PS, .module_handle = s_fragment_shader, .entry_point = "FS_Main" },
    };

    rhi::Format colour_format = present_desc.format;

    s_pipeline = rhi::CreatePipelineState({
        .type           = rhi::PipelineType::Graphics,
        .shader_stages  = stages,
        .raster_state = {
            // RasterRenderState::cull_mode defaults to Back — explicitly
            // disable culling for this full-screen triangle.
            .cull_mode = rhi::RasterCullMode::None,
        },
        .prim_type      = rhi::PrimitiveType::TriangleList,
        .render_pass_info = {
            .color_attachments = Span<rhi::Format>(&colour_format, 1),
        },
    });

    PHX_LOG_INFO(k_log, "Initialized");
    return true;
}

void phx::ToneMapBlit::Shutdown()
{
    if (s_pipeline.IsValid())
        rhi::DestroyPipelineState(s_pipeline);

    if (s_vertex_shader.IsValid())
        rhi::DestroyShaderModule(s_vertex_shader);

    if (s_fragment_shader.IsValid())
        rhi::DestroyShaderModule(s_fragment_shader);

    PHX_LOG_INFO(k_log, "Shutdown complete");
}

void phx::ToneMapBlit::Blit(rhi::TextureHandle source, rhi::CommandBuffer cmd, float exposure)
{
    if (!s_pipeline.IsValid())
    {
        PHX_LOG_ERROR(k_log, "Blit called before a successful Initialize()");
        return;
    }

    const rhi::DescriptorIndex scene_colour_index = rhi::GetShaderResourceIndex(source);
    if (scene_colour_index == rhi::kInvalidDescriptorIndex)
    {
        PHX_LOG_ERROR(k_log, "Blit source has no shader-resource-view — was it created with BindingFlags::ShaderResource?");
        return;
    }

    rhi::Barrier(cmd, rhi::BarrierStage::Graphics, rhi::BarrierStage::Graphics);

    rhi::BeginRenderPass({}, cmd);
    rhi::BindPipelineState(s_pipeline, cmd);

    PushConstants push_constants = {
        .scene_colour_index = scene_colour_index,
        .exposure           = exposure,
    };
    rhi::SetPushConstants(cmd, &push_constants, sizeof(push_constants));

    rhi::Draw(cmd, 3);

    rhi::EndRenderPass(cmd);
}
