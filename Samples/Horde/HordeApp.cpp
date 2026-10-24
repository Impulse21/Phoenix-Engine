#include "HordeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Runtime.h>

#include <PhxEngine/ECS/EntityId.h>
#include <PhxEngine/ECS/LinearStorage.h>
#include <PhxEngine/ECS/SparseSet.h>

#include "WorldComponents.h"
#include "HordeLevelFactory.h"

using namespace phx;
using namespace horde;

namespace
{
    // TODO: I don't think I need this
    enum RenderViewKind : u32
    {
        Camera = 0,
        SunShadow,
    };
}  // namespace

PHX_DEFINE_APP(HordeApp);

PHX_CVAR_BOOL(use_debug_level, false, "Flag to switch to debug level for GPU debugging");

const char* horde::HordeApp::GetName() const { return "PhxHorde"; }

void horde::HordeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);
    VFS::Mount("assets_core://", PHX_ASSET_CORE_SOURCE_DIR);

    PHX_LOG_INFO(Log::Channels::App, "Initializing Renderer");
    if (!m_renderer.Initialize())
    {
        PHX_LOG_ERROR(Log::Channels::App, "HordeRenderer::Initialize failed");
        return;
    }

    if (CVar_use_debug_level.Get())
    {
        PHX_LOG_INFO(Log::Channels::App, "Buildinging primitives test");
        BuildPrimitiveTests(m_world);
    }
    else
    {
        PHX_LOG_INFO(Log::Channels::App, "Building blockout level");
        m_main_camera_id = BuildBlockoutLevel(m_world);
    }
}

void horde::HordeApp::OnShutdown()
{
    m_renderer.Shutdown();
}

void horde::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] {
        FrameAllocator& allocator = phx::Memory::GetFrameAlloc();
        ScratchAllocator& scratch = phx::Memory::GetScratchAlloc();

        m_frame_data = allocator.Alloc<FrameData>();
        m_frame_data->allocator = &allocator;
        m_frame_data->scratch = &scratch;


        // include Build Views
        // Build Views
        m_renderer.PreRender(m_world, m_main_camera_id, *m_frame_data);

    });

}

void horde::HordeApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt)
{
    graph.Emplace([this, dt] {
        Update(dt);
    });
}

void horde::HordeApp::OnBuildRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] {
        m_renderer.Render(*m_frame_data);
    });
}

void horde::HordeApp::Update(float)
{
}

