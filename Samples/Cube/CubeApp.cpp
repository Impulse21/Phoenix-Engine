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

#include <utility>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit() 
{
    m_command_buffer = phx::rhi::CreateCommandBuffer({
        .type = phx::rhi::CommandQueueType::Graphics,
    });

    ShaderCompiler::Initialize();

    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);

    auto vs_result = ShaderCompiler::Compile("shaders://Cube.slang", "VS_Main", ShaderCompiler::Stage::Vertex);
    auto fs_result = ShaderCompiler::Compile("shaders://Cube.slang", "FS_Main", ShaderCompiler::Stage::Fragment);

    if (!vs_result || !fs_result)
    {
        PHX_LOG_ERROR(Log::Channels::App, "Failed to compile Cube.slang");
        return;
    }

    m_vertex_shader_spirv   = std::move(vs_result.GetValue());
    m_fragment_shader_spirv = std::move(fs_result.GetValue());

    ToneMapBlit::Initialize();
}

void SceneColourCallback(rhi::CommandBufferHandle)
{

}

void PresentCallback(rhi::CommandBufferHandle)
{

}

void samples::CubeApp::OnPreRender() 
{
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnRender(const phx::FrameRenderTargets& targets) 
{
    phx::rhi::BeginCommandRecording(m_command_buffer);

    phx::rhi::BeginRenderPass(
        targets.scene_colour,
        { .colour = { 0.0f, 0.0f, 1.0f, 1.0f }},
        targets.depth,
        { .depth_stencil = { .depth = 1.0f }},
        m_command_buffer
    );

    phx::rhi::EndRenderPass(m_command_buffer);

    ToneMapBlit::Blit(targets.scene_colour, m_command_buffer);
}
                
void samples::CubeApp::OnShutdown()
{
    phx::rhi::DestoryCommandBuffer(m_command_buffer);

    ToneMapBlit::Shutdown();
    phx::ShaderCompiler::Shutdown();
}
