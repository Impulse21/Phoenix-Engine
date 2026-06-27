#include "Engine.h"

#include <PhxEngine/IApplication.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Core/Thread.h>

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

    rhi::ViewportHandle s_viewport;
}

void phx::Engine::Initialize(IApplication* app, Span<char*> args) 
{
    PHX_ASSERT(app);

    s_app = app;
    s_running = true;

    CVar::Initialize(args);

    // Logging doesn't use memory allocators, so init first to allow logging during engine init
    Log::Initialize();
    PHX_LOG_INFO(Log::Channels::Engine, "Initialising PhxEngine");

    Thread::Initialize();
    Memory::Initialize();

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
            .heap_allocator = &Memory::g_Heap,
            .app_name = s_app->GetName(),
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

        // -- TODO: Move to renderer
        s_viewport = rhi::CreateViewport({
            .window_handle          = s_window,
            .width                  = static_cast<u32>(CVar_engine_window_width.Get()), 
            .height                 = static_cast<u32>(CVar_engine_window_height.Get()),
            .fullscreen             = CVar_rhi_enable_fullscreen.Get(),
            .v_sync                 = CVar_rhi_enable_vsync.Get(),
            .enable_hdr             = CVar_rhi_enable_hdr.Get(),
        });
    }

#if false
        PHX_LOG_INFO(Log::Channels::Engine, "Initialising PHX — {}", desc.appName);

    if (!Jobs::Init(desc.jobThreads))
    {
        PHX_LOG_ERROR(Log::Channels::Engine, "Failed to initialise job system");
        return false;
    }

    if (!desc.headless)
    {
        if (!Platform::Init(desc))
        {
            PHX_LOG_ERROR(Log::Channels::Engine, "Failed to initialise platform");
            return false;
        }

        if (!RHI::Init(Platform::GetWindow()))
        {
            PHX_LOG_ERROR(Log::Channels::Engine, "Failed to initialise RHI");
            return false;
        }
    }

    // App inits last — engine is fully booted before game code runs
    app->OnInit(Memory::GetDefault());

    s_running = true;

    PHX_LOG_INFO(Log::Channels::Engine, "Init complete");
    return true;
#endif
}

void phx::Engine::Run() 
{
    PHX_ASSERT(s_app != nullptr);
    PHX_ASSERT(s_running);

    while (s_running)
    {
        phx::platform::PollEvents();

        if (s_window.IsValid() && phx::platform::ShouldClose(s_window))
        {
            RequestExit();
            continue;
        }
        
        renderer::RenderGraphBuilder rg_builder;
        RenderWorld render_world = {};
        s_app->OnBuildGraph(rg_builder, render_world);

        // -- Update thread ---
        s_app->OnUpdate(0.f);

        // -- Render thread ---
        bool can_render = rhi::BeginFrame(s_viewport);
        if (can_render)
        {
            s_app->OnRender();
            rhi::EndFrame(s_viewport);
        }

        s_frame_idx ^= 1;
    }

#if false

    PHX_ASSERT_BASIC(s_app != nullptr);
    PHX_ASSERT_BASIC(s_running);

    while (s_running)
    {
        if (!s_app->GetDesc().headless)
        {
            Platform::PollEvents();
            if (Platform::ShouldClose())
                break;
        }

        const f32 dt       = Time::Tick();
        const u32 writeIdx = s_frameIdx;
        const u32 readIdx  = writeIdx ^ 1;

        // ── Cache step (sync, main thread) ────────────────────────────────────
        // Safe to reset — render job from previous frame is done
        Memory::GetFrameAlloc()->Reset();

        s_renderWorld[writeIdx] = {};
        s_app->OnFillWorld(s_renderWorld[writeIdx]);

        RG::Builder builder;
        s_app->OnBuildGraph(builder, s_renderWorld[writeIdx]);

        RG::Release(s_graph[writeIdx]);
        s_graph[writeIdx] = RG::Compile(builder);

        // ── Parallel phase ────────────────────────────────────────────────────
        RenderWorld*      readWorld = &s_renderWorld[readIdx];
        RG::CompiledGraph* readGraph = s_graph[readIdx];

        JobHandle updateJob = Jobs::Kick([dt]()
        {
            s_app->OnUpdate(dt);
        });

        JobHandle renderJob = Jobs::Kick([readWorld, readGraph]()
        {
            RHI::CommandList* cmd = RHI::BeginFrame();
            RG::Execute(readGraph, cmd, *readWorld);
            RHI::EndFrame(cmd);
        });

        Jobs::WaitAll({ updateJob, renderJob });

        if (!s_app->GetDesc().headless)
            RHI::Present();

        s_frameIdx ^= 1;
    }
#endif
}

void phx::Engine::Shutdown() 
{
    PHX_LOG_INFO(Log::Channels::Engine, "Shutting down PhxEngine");

    // -- TODO: Move to renderer ---
    if (s_viewport.IsValid())
        phx::rhi::DestoryViewport(s_viewport);

    phx::rhi::Shutdown();
    if (s_window.IsValid())
    {
        phx::platform::DestroyOSWindow(s_window);
    }

    Memory::Shutdown();
    Log::Shutdown();
    CVar::Shutdown();
}

void phx::Engine::RequestExit() 
{
    s_running = false;
}
