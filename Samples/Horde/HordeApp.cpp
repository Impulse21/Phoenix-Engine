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
    // m_renderer.Shutdown();
}

void horde::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph)
{
    FrameAllocator& allocator = phx::Memory::GetFrameAlloc();
    ScratchAllocator& scratch = phx::Memory::GetScratchAlloc();

    m_frame_data = allocator.Alloc<FrameData>();
    m_frame_data->allocator = &allocator;
    m_frame_data->scratch = &scratch;


    // include Build Views
    // Build Views
    m_renderer.PreRender(m_world, m_main_camera_id, *m_frame_data);

    // Process Primitives
    // Build Data for rendering, this doesn't seem like a good spot for this:

    // We are going to start off with only a plan so we can tri out Tri Planner Projections 
    // of Texture Asset
    
    // TODO: I am here, Need to process Planes
    // Set up cachine

    #if false
    {
        const u32 num_planes = static_cast<u32>(m_world.Count<PlaneRenderComponent>());
        if (num_planes > 0)
        {
            GpuCpuRange<shader_interop::PlaneInstanceData> instances =
                gpu_frame_allocator.Alloc<shader_interop::PlaneInstanceData>(num_planes);

            u32 curr_index = 0;
            m_world.Each<PlaneRenderComponent, TransformComponent>(
                [&](ecs::EntityId e, const PlaneRenderComponent& plane, const TransformComponent& transform)
            {
                PHX_ASSERT(curr_index < num_planes && "Count() didn't match what Each() visited");
                if (curr_index >= num_planes)
                    return;

                shader_interop::PlaneInstanceData& instance = instances.cpu[curr_index++];
                instance.model    = ToMatrix(transform);
                instance.colour = ColourFor(world, e);
                instance.extent = plane.extent;
            });


            GpuCpuRange<shader_interop::PlaneDrawRoot> draw_root =
                gpu_frame_allocator.Alloc<shader_interop::PlaneDrawRoot>();

            draw_root->frame_data = frame_data.gpu;
            draw_root->instances = instances.gpu;
            
            m_curr_render_list->render_packets[m_curr_render_list->num_render_packets++] = {
                .pso_handle     = m_pso[Pso::Plane],
                .draw_root      = draw_root.ToGpuRange(),
                .instance_count = curr_index,
            };
        }
    }

    // TODO: Use threading at some point
    // not required while we only have a single view
    graph.ForEachIndex(
        0,
        static_cast<u32>(frame_data->views.Size()),
        [this, frame_data]{

            // -- TODO, I will need the world to set this up
            CullAndCopy();
        });
    #endif

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
        m_renderer.Render();
    });
}

void horde::HordeApp::Update(float)
{
}

