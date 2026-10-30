#include "Runtime.h"

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

    int64_t s_last_frame_tick = 0;
}


void phx::Runtime::Initialize(IApplication* app, Span<char*> args) 
{
    PHX_ASSERT(app);

    PHX_PROFILE_SCOPE

    s_app = app;
    s_running = true;

    CVar::Initialize(args);

    // Logging doesn't use memory allocators, so init first to allow logging during engine init
    Log::Initialize();
    PHX_LOG_INFO(Log::Channels::Engine, "Initialising PhxEngine");

    Thread::Initialize();
    SystemTime::Initialize();
    Memory::Initialize();
    Jobs::Initialize();
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
    }

    s_running = true;

    app->OnInit();

    PHX_LOG_INFO(Log::Channels::Engine, "Init complete");    
}

void phx::Runtime::Run() 
{
    PHX_ASSERT(s_app != nullptr);
    PHX_ASSERT(s_running);

    s_last_frame_tick = SystemTime::GetCurrentTick();

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

        if (!rhi::BeginFrame())
            return;

        s_app->FramePreRender();

        // TODO: Wait on a frame barrier.
        
        Jobs::Wait();

        Jobs::Barrier frame_done;
        Jobs::Submit([dt]() {
            s_app->FrameUpdate(dt);
        }, frame_done);
        
        s_app->FrameRender();

        Jobs::Wait(frame_done);
        
        s_frame_idx ^= 1;
    }
}

void phx::Runtime::Shutdown() 
{
    PHX_PROFILE_SCOPE
    PHX_LOG_INFO(Log::Channels::Engine, "Shutting down PhxEngine");

    s_app->OnShutdown();

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

void phx::Runtime::RequestExit()
{
    s_running = false;
}
