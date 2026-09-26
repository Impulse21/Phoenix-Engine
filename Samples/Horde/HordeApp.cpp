#include "HordeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(HordeApp);

const char* samples::HordeApp::GetName() const { return "PhxHorde"; }

void samples::HordeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);
}

void samples::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph&)
{
}

void samples::HordeApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt)
{
    graph.Emplace([this, dt] {
        Update(dt);
    });
}

void samples::HordeApp::OnBuildRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] {
        Render();
    });
}

void samples::HordeApp::Update(float)
{
}

void samples::HordeApp::Render()
{
    rhi::CommandBuffer cmd = rhi::BeginCommandRecording(rhi::CommandQueueType::Graphics);

    rhi::CmdBeginRenderPass({ .colour = { 0.05f, 0.05f, 0.08f, 1.0f } }, cmd);
    rhi::CmdEndRenderPass(cmd);

    rhi::SubmitAndPresent(Span<rhi::CommandBuffer>(&cmd, 1));
}

void samples::HordeApp::OnShutdown()
{
}
