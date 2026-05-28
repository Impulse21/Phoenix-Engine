#include "Engine.h"

#include <PhxEngine/IApplication.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>

using namespace phx;

namespace
{
    IApplication*   s_app       = nullptr;
    bool            s_running   = false; 

    u32             s_frame_idx = 0;
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

    Memory::Initialize();

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
        RenderWorld render_world = {};
        s_app->OnFillWorld(render_world);
        s_app->OnUpdate(0.f);
        s_app->OnRender();
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

    Memory::Shutdown();
    Log::Shutdown();
    CVar::Shutdown();
}

void phx::Engine::RequestExit() 
{
    s_running = false;
}
