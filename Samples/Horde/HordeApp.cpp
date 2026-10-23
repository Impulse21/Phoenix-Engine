#include "HordeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>
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

const char* samples::HordeApp::GetName() const { return "PhxHorde"; }

void samples::HordeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);

#if false
    PHX_LOG_INFO(Log::Channels::App, "Initializing Renderer");
    if (!m_renderer.Initialize())
    {
        PHX_LOG_ERROR(Log::Channels::App, "HordeRenderer::Initialize failed");
        return;
    }

#endif

    // Initalize Render graph registry
    // Maybe give heap space to this bit.
    // m_rg_registry.Initialize();

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

void samples::HordeApp::OnShutdown()
{
    // m_renderer.Shutdown();
}

void samples::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph, FrameData* frame_data)
{
    // include Build Views
    // Build Views
    BuildViews(frame_data);

    // Only deal with camera for right now.
    PHX_ASSERT(frame_data->views.size() == 1);

    // Process Primitives
    // Build Data for rendering, this doesn't seem like a good spot for this:

    // We are going to start off with only a plan so we can tri out Tri Planner Projections 
    // of Texture Asset
    
    // TODO: I am here, Need to process Planes
    // Set up cachine
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
    #if false
    graph.ForEachIndex(
        0,
        static_cast<u32>(frame_data->views.Size()),
        [this, frame_data]{

            // -- TODO, I will need the world to set this up
            CullAndCopy();
        });
    #endif

}

void samples::HordeApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, const FrameData* frame_data)
{
    graph.Emplace([this, frame_data] {
        Update(frame_data->delta);
    });
}

void samples::HordeApp::OnBuildRenderFrame(phx::Jobs::Graph& graph, const FrameData* frame_data)
{
    graph.Emplace([this] {
        // m_renderer.Render();
    });
}

void samples::HordeApp::Update(float)
{
}

void samples::HordeApp::BuildViews(FrameData* frame_data) const
{
    // Build Views
    FrameAllocator* frame_allocator = frame_data->allocator;

    // -- Process Main Camera ---
    FrameList<phx::renderer::RenderView> camera_render_views(*frame_allocator, 1);
    {
        auto* camera = m_world.TryGet<CameraComponent>(m_main_camera_id);
        auto* transform = m_world.TryGet<TransformComponent>(m_main_camera_id);
        PHX_ASSERT(camera && transform);

        rhi::ViewportDesc viewport_desc;
        rhi::GetViewportDesc(viewport_desc);

        phx::renderer::RenderView render_view = {
            .Kind = RenderViewKind::Camera,
        };

        // -- Calculate aspect Ration --
        const float aspect =
            static_cast<float>(viewport_desc.width) / static_cast<float>(viewport_desc.height);

        render_view.view = hlslpp::float4x4::look_at(transform->position, camera->target, camera->up);

        // Calculate frustum
        const hlslpp::frustum frustum =
            hlslpp::frustum::field_of_view_y(hlslpp::radians(hlslpp::float1(camera->fov_y_degrees)),
                aspect,
                camera->near_plane,
                camera->far_plane);

        const hlslpp::projection proj_params(
            frustum, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);

        render_view.proj = hlslpp::float4x4::perspective(proj_params);
        if (rhi::IsClipSpaceYDown())
            render_view.proj = hlslpp::mul(render_view.proj, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));

        render_view.view_proj = hlslpp::mul(render_view.view, render_view.proj);

        camera_render_views.Push(std::move(render_view));
    }

    frame_data->views = camera_render_views.ToSpan();

    // -- TODO Lights ---
#if false
    FrameList<phx::renderer::VisibileLight> visibile_lights(*frame_allocator, 1);
    m_world.Each<LightComponent, TransformComponent>(
        [&](ecs::EntityId e, const LightComponent& light, const TransformComponent& transform)
    {
        if (!light.casts_shadows)
            return;

        PHX_ASSERT(light.type == LightType::Directional);   // only directional for now

        visibile_lights.Push(renderer::VisibileLight{
            .light_index = 0,
            .first_view = 0,
            .view_count = 1
        });
        const float half_extent = light.shadow_distance;
        const u32   resolution  = light.shadow_resolution;
        if (half_extent <= 0.0f || resolution == 0)
            return;

        const hlslpp::float3 light_dir =
            hlslpp::mul(transform.rotation, hlslpp::float3(0.0f, 0.0f, -1.0f));

        // Shadow camera sits half_extent behind the focus, looking at it
        const hlslpp::float3 light_position = shadow_focus - light_dir * half_extent;

        const hlslpp::float3 world_up(0.0f, 1.0f, 0.0f);
        const hlslpp::float3 up =
            (float(hlslpp::abs(hlslpp::dot(light_dir, world_up))) > 0.99f)   // verify float1 -> float
                ? hlslpp::float3(1.0f, 0.0f, 0.0f)
                : world_up;

        const hlslpp::float4x4 view =
            hlslpp::float4x4::look_at(light_position, shadow_focus, up);

        // Box is 2*half_extent square, depth 0 .. 2*half_extent, centered on the focus
        const float size = 2.0f * half_extent;
        const hlslpp::frustum box(size, size, 0.0f, size);                    // verify ctor: width, height, near, far
        const hlslpp::projection proj_params(
            box, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);

        hlslpp::float4x4 proj = hlslpp::float4x4::orthographic(proj_params);  // verify

        // Fold the flip into proj so proj and view_proj agree
        if (rhi::IsClipSpaceYDown())
            proj = hlslpp::mul(proj, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));

        const hlslpp::float4x4 view_proj = hlslpp::mul(view, proj);

        views.cpu[view_counter++] = phx::renderer::RenderView{
            .kind      = RenderViewKind::SunShadow,
            .proj      = proj,
            .view      = view,
            .view_proj = view_proj,
            .viewport  = { .x = 0, .y = 0, .width = resolution, .height = resolution },
        };
    });
#endif

}
