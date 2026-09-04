#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/Renderer/ToneMapBlit.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include <utility>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit()
{
    ShaderCompiler::Initialize();

    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);

    auto vs_result = ShaderCompiler::Compile("shaders://Cube.slang", "VS_Main", ShaderCompiler::Stage::Vertex);
    auto fs_result = ShaderCompiler::Compile("shaders://Cube.slang", "FS_Main", ShaderCompiler::Stage::Fragment);

    if (!vs_result || !fs_result)
    {
        PHX_LOG_ERROR(Log::Channels::App, "Failed to compile Cube.slang");
        return;
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

    rhi::Format colour_format = phx::Engine::GetColourBufferFormat();
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
            .depth_stencil_format = phx::Engine::GetDepthBufferFormat(),
        },
    });

    ToneMapBlit::Initialize();
}

void SceneColourCallback(rhi::CommandBuffer)
{

}

void PresentCallback(rhi::CommandBuffer)
{

}

void samples::CubeApp::OnPreRender() 
{
}

void samples::CubeApp::OnUpdate(float dt)
{
    m_time += dt;
}

phx::rhi::CommandBuffer samples::CubeApp::OnRender(const phx::FrameRenderTargets& targets)
{
    rhi::ViewportDesc viewport_desc;
    rhi::GetViewportDesc(viewport_desc);
    const float aspect = static_cast<float>(viewport_desc.width) / static_cast<float>(viewport_desc.height);

    const hlslpp::float1 t = m_time;
    const hlslpp::float3 eye = hlslpp::float3(hlslpp::sin(t), hlslpp::float1(0.6f), hlslpp::cos(t)) * 2.5f;
    const hlslpp::float4x4 view = hlslpp::float4x4::look_at(eye, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f));

    const hlslpp::frustum frustum = hlslpp::frustum::field_of_view_y(hlslpp::radians(hlslpp::float1(60.0f)), aspect, 0.1f, 100.0f);
    const hlslpp::projection proj_params(frustum, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);
    hlslpp::float4x4 proj = hlslpp::float4x4::perspective(proj_params);

    struct DrawData
    {
        hlslpp::float4x4 mvp;
    } data;

    // hlslpp's default HLSLPP_LOGICAL_LAYOUT is row-major: look_at/perspective
    // build matrices for "vector * matrix" (row-vector-on-left), so matrices
    // chain left-to-right in application order (model * view * proj) and the
    // shader must use mul(vector, matrix) to match — see Cube.slang.
    hlslpp::float4x4 mvp = hlslpp::mul(view, proj); // model is identity

    // Vulkan's clip space is Y-down; hlslpp's generic perspective builder
    // assumes Y-up. A diagonal scale matrix is transpose-invariant, so this
    // is correct under row-vector chaining without needing to reason about
    // which row/column actually carries the output's Y component.
    if (rhi::IsClipSpaceYDown())
        mvp = hlslpp::mul(mvp, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));

    data.mvp = mvp;

    phx::rhi::CommandBuffer cmd = phx::rhi::BeginCommandRecording(phx::rhi::CommandQueueType::Graphics);

    phx::rhi::BeginRenderPass(
        targets.scene_colour,
        { .colour = { 0.0f, 0.0f, 0.0f, 1.0f }},
        targets.depth,
        { .depth_stencil = { .depth = 1.0f }},
        cmd
    );

    phx::rhi::BindPipelineState(m_cube_pipeline, cmd);

    // Pushed directly — vkCmdPushConstants copies these bytes into the
    // command buffer at record time, so there's no need to stage them
    // through a GPU allocation first.
    phx::rhi::SetPushConstants(cmd, &data, sizeof(data));
    phx::rhi::Draw(cmd, 36);

    phx::rhi::EndRenderPass(cmd);

    ToneMapBlit::Blit(targets.scene_colour, cmd);

    return cmd;
}

void samples::CubeApp::OnShutdown()
{
    ToneMapBlit::Shutdown();
    phx::ShaderCompiler::Shutdown();
}
