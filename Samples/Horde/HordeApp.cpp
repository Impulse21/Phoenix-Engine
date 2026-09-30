#include "HordeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include <PhxEngine/ECS/EntityId.h>
#include <PhxEngine/ECS/LinearStorage.h>
#include <PhxEngine/ECS/SparseSet.h>

#include "WorldComponents.h"
#include "HordeLevelFactory.h"

using namespace samples;
using namespace phx;
using namespace horde;


PHX_DEFINE_APP(HordeApp);

const char* samples::HordeApp::GetName() const { return "PhxHorde"; }

void samples::HordeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);

    PHX_LOG_INFO(Log::Channels::App, "Initializing Renderer");
    if (!m_renderer.Initialize())
    {
        PHX_LOG_ERROR(Log::Channels::App, "HordeRenderer::Initialize failed");
        return;
    }

    PHX_LOG_INFO(Log::Channels::App, "Building blockout level");
    BuildBlockoutLevel(m_world);
    
}

void samples::HordeApp::OnShutdown()
{
    m_renderer.Shutdown();
}

void samples::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this]() {
        m_renderer.PreRender(this->m_world, Memory::GetFrameAlloc());
    });
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
        m_renderer.Render();
    });
}

void samples::HordeApp::Update(float)
{
}

