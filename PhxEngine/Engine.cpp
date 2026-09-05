#include "Engine.h"

#include <PhxEngine/IApplication.h>

#include <PhxEngine/Core/Profile.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Core/Thread.h>
#include <PhxEngine/Core/SystemTime.h>
#include <PhxEngine/Core/Jobs.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Memory/Memory.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/OSWindow.h>

using namespace phx;

PHX_CVAR_INT(engine_window_width, 1280, "Initial window width");
PHX_CVAR_INT(engine_window_height, 720, "Initial window height");
PHX_CVAR_BOOL(engine_headless, false, "Run the engine without creating a window or initializing the RHI. Useful for dedicated servers or command-line tools.");

PHX_CVAR_BOOL(rhi_enable_validation, false, "Enable RHI validation layers (if supported by the platform)");
PHX_CVAR_BOOL(rhi_enable_best_practices, true, "Enable best practices validation (if supported by the platform)");
PHX_CVAR_BOOL(rhi_enable_sync_validation, false, "Enable synchronization validation (if supported by the platform)");
PHX_CVAR_BOOL(rhi_enable_gpu_assisted, false, "Enable GPU assisted validation (if supported by the platform) (Warning: Very Expensive!)");

PHX_CVAR_BOOL(rhi_enable_fullscreen, false, "Enable Fullscreen swapchain");
PHX_CVAR_BOOL(rhi_enable_vsync, false, "Enable v-sync");
PHX_CVAR_BOOL(rhi_enable_hdr, false, "Enable enables HDR (if supported by montor)");

namespace
{
    IApplication*   s_app       = nullptr;
    bool            s_running   = false; 

    u32             s_frame_idx = 0;

    phx::platform::OSWindowHandle s_window;

    std::array<FrameRenderTargets, rhi::MaxFramesInFlight> s_frame_render_targets;

    int64_t s_last_frame_tick = 0;

    // Long-lived, reused every frame via Clear() -- see IApplication.h for
    // the PreRender/Update/Render ordering contract.
    Jobs::Graph s_pre_render_graph;
    Jobs::Graph s_update_graph;
    Jobs::Graph s_render_graph;
    Jobs::Graph s_frame_graph;
}

void phx::Engine::Initialize(IApplication* app, Span<char*> args) 
{
    PHX_ASSERT(app);

    PHX_PROFILE_SCOPE();
    s_app = app;
    s_running = true;

    CVar::Initialize(args);

    // Logging doesn't use memory allocators, so init first to allow logging during engine init
    Log::Initialize();
    PHX_LOG_INFO(Log::Channels::Engine, "Initialising PhxEngine");

    Thread::Initialize();
    SystemTime::Initialize();
    s_last_frame_tick = SystemTime::GetCurrentTick();

    // Memory::Initialize() must run before Jobs::Initialize() -- Jobs
    // workers start immediately inside the Executor constructor, and each
    // one calls Memory::InitializeThreadLocal() (via the worker-start hook
    // below) as soon as it starts, which needs Memory::g_Arena to already
    // exist.
    Memory::Initialize();

    Jobs::Initialize(0,
        []() { Memory::InitializeThreadLocal(); },
        []() { Memory::ShutdownThreadLocal(); });

    VFS::Initialize();
    VFS::Mount("engine_shaders://", PHX_ENGINE_SHADER_DIR);

    if (CVar_engine_headless.Get())
    {
        PHX_LOG_INFO(Log::Channels::Engine, "Running in headless mode (no window or RHI)");
    }
    else
    {
        s_window = phx::platform::CreateOSWindow({ 
            .width = static_cast<u32>(CVar_engine_window_width.Get()), 
            .height = static_cast<u32>(CVar_engine_window_height.Get()), 
            .title = s_app->GetName()
        });

        bool success = phx::rhi::Initialize({
            .app_name = s_app->GetName(),
            .viewport = {
                .window_handle = s_window,
                .width         = static_cast<u32>(CVar_engine_window_width.Get()),
                .height        = static_cast<u32>(CVar_engine_window_height.Get()),
                .fullscreen    = CVar_rhi_enable_fullscreen.Get(),
                .v_sync        = CVar_rhi_enable_vsync.Get(),
                .enable_hdr    = CVar_rhi_enable_hdr.Get(),
            },
            .enable_validation = CVar_rhi_enable_validation.Get(),
            .enable_best_practices = CVar_rhi_enable_best_practices.Get(),
            .enable_sync_validation = CVar_rhi_enable_sync_validation.Get(),
            .enable_gpu_assisted = CVar_rhi_enable_gpu_assisted.Get()});

        if (!success)
        {
            PHX_LOG_ERROR(
                Log::Channels::Engine,
                "Failed to initialize RHI. Exiting application");
            std::abort();
        }

        for (u32 i = 0; i < s_frame_render_targets.size(); ++i)
        {
            rhi::TextureHandle colour_target = rhi::CreateTexture({
                .debug_name             = "colour_target",
                .format                 = GetColourBufferFormat(),
                .width                  = static_cast<u32>(CVar_engine_window_width.Get()), 
                .height                 = static_cast<u32>(CVar_engine_window_height.Get()),
                .clear_value            = { .colour { 1.0f, 1.0f, 1.0f, 1.0f} },
                .binding_flags          = rhi::BindingFlags::RenderTarget | rhi::BindingFlags::ShaderResource,
                .initial_state          = rhi::ResourceStates::RenderTarget,
            });

            rhi::TextureHandle depth_target = rhi::CreateTexture({
                .debug_name             = "depth_target",
                .format                 = GetDepthBufferFormat(),
                .width                  = static_cast<u32>(CVar_engine_window_width.Get()), 
                .height                 = static_cast<u32>(CVar_engine_window_height.Get()),
                .clear_value            = { .depth_stencil = { 0.0f }},
                .binding_flags          = rhi::BindingFlags::DepthStencil,
                .initial_state          = rhi::ResourceStates::DepthWrite,
            });

            s_frame_render_targets[i] = {
                .scene_colour = colour_target,
                .depth = depth_target,
            };
        }
    }

    s_running = true;

    app->OnInit();
    PHX_LOG_INFO(Log::Channels::Engine, "Init complete");
}

void phx::Engine::Run() 
{
    PHX_ASSERT(s_app != nullptr);
    PHX_ASSERT(s_running);

    while (s_running)
    {
        PHX_PROFILE_FRAME();
        phx::platform::PollEvents();

        if (s_window.IsValid() && phx::platform::ShouldClose(s_window))
        {
            RequestExit();
            continue;
        }
        
        Memory::BeginFrame();

        const int64_t now_tick = SystemTime::GetCurrentTick();
        const float dt = static_cast<float>(SystemTime::TimeBetweenTicks(s_last_frame_tick, now_tick));
        s_last_frame_tick = now_tick;

        s_pre_render_graph.Clear();
        s_update_graph.Clear();
        s_render_graph.Clear();
        s_frame_graph.Clear();
        
        if (!rhi::BeginFrame())
            return;

        const u32 current_target_idx = s_frame_idx % rhi::MaxFramesInFlight;

        rhi::CommandBuffer cmd;
        s_app->OnBuildPreRenderFrame(s_pre_render_graph);
        s_app->OnBuildUpdateFrame(s_update_graph, dt);
        s_app->OnBuildRenderFrame(s_render_graph, s_frame_render_targets[current_target_idx], cmd);

        Jobs::TaskHandle pre_render = s_frame_graph.ComposeOf(s_pre_render_graph);
        Jobs::TaskHandle update     = s_frame_graph.ComposeOf(s_update_graph);
        Jobs::TaskHandle render     = s_frame_graph.ComposeOf(s_render_graph);
        s_frame_graph.Precede(pre_render, update);
        s_frame_graph.Precede(pre_render, render);

        Jobs::RunAndWait(s_frame_graph);

        rhi::SubmitAndPresent(Span<rhi::CommandBuffer>(&cmd, 1));

        s_frame_idx ^= 1;
    }
}

void phx::Engine::Shutdown() 
{
    PHX_PROFILE_SCOPE();
    PHX_LOG_INFO(Log::Channels::Engine, "Shutting down PhxEngine");

    s_app->OnShutdown();

    // -- TODO: Move to renderer ---
    for (auto& target : s_frame_render_targets)
    {
        rhi::DestroyTexture(target.scene_colour);
        rhi::DestroyTexture(target.depth);
    }

    // -- End TODO ---
    // Viewport teardown happens inside rhi::Shutdown() — it's owned by the
    // context, not a separate resource the app destroys.
    phx::rhi::Shutdown();
    if (s_window.IsValid())
    {
        phx::platform::DestroyOSWindow(s_window);
    }
    
    Jobs::Shutdown();
    VFS::Shutdown();
    Memory::Shutdown();
    Log::Shutdown();
    CVar::Shutdown();
}

void phx::Engine::RequestExit()
{
    s_running = false;
}

rhi::Format phx::Engine::GetColourBufferFormat()
{
    return rhi::Format::RGBA16_FLOAT;
}

rhi::Format phx::Engine::GetDepthBufferFormat()
{
    return rhi::Format::D32;
}
