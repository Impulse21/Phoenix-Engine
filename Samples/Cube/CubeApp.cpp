#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Renderer/RenderGraph.h>
#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit() 
{
    m_command_buffer = phx::rhi::CreateCommandBuffer({
        .type = phx::rhi::CommandQueueType::Graphics,
    });
}

void SceneColourCallback(rhi::CommandBufferHandle)
{

}

void PresentCallback(rhi::CommandBufferHandle)
{

}

void samples::CubeApp::OnBuildGraph(renderer::RenderGraphBuilder& rg_builder, RenderWorld& world) 
{
    PHX_UNUSED(world);

    renderer::GraphResource scene_colour = rg_builder.DeclareResource(
        renderer::ResourceDesc::Texture({
            .width = 0, .height = 0, // zero to match main viewport
            .format = rhi::Format::RGBA16_FLOAT
        })
    );

    rg_builder.AddPass(
            "Scene Colour",
            [scene_colour](renderer::PassBuilder& pass) 
            {
                pass.Write(scene_colour);
            },
            &SceneColourCallback);

    rg_builder.AddPass(
            "Present",
            [scene_colour, &rg_builder](renderer::PassBuilder& pass) 
            {
                pass.Read(scene_colour);
                pass.Write(rg_builder.GetBackBuffer());
            },
            &SceneColourCallback);
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnRender() 
{
    phx::rhi::BeginCommandRecording(m_command_buffer);

    phx::rhi::ViewportHandle main_viewport = phx::Engine::GetViewport();

    phx::rhi::BeginRendering(
        main_viewport,
        { .colour = { 0.0f, 0.0f, 1.0f, 1.0f }},
        m_command_buffer
    );

    phx::rhi::EndRendering(m_command_buffer);
}

void samples::CubeApp::OnShutdown() 
{
    phx::rhi::DestoryCommandBuffer(m_command_buffer);
}
