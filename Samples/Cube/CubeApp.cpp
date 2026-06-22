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

namespace RenderPasses
{
    void ClearPassCallback(rhi::CommandBufferHandle cmd_handle, void* user_data)
    {

    };
}
void samples::CubeApp::OnInit() 
{
}

void samples::CubeApp::OnBuildGraph(renderer::RenderGraphBuilder& rg_builder, RenderWorld& world) 
{
    rg_builder.AddPass("Clear Back Buffer", &RenderPasses::ClearPassCallback, nullptr,
        [&](renderer::PassBuilder& pass_builder)
    {
        PHX_LOG_INFO(Log::Channels::App, "Clearing Render Target");
    });
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnRender() 
{

}

void samples::CubeApp::OnShutdown() 
{

}
