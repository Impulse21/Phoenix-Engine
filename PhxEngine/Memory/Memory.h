#pragma once

#include <PhxEngine/Memory/VirtualMemoryArena.h>

#include "VirtualMemoryArena.h"
#include "FrameAllocator.h"
#include "ScratchAllocator.h"

namespace phx
{
    namespace Memory
    {
        inline VirtualMemoryArena   g_Arena;

        enum class ArenaType { Virtual, };

        // Sets up g_Arena and this (the calling) thread's own frame/scratch
        // allocators. Call once, on whichever thread drives Engine::Initialize.
        void Initialize();
        void Shutdown();

        // Sets up / tears down frame/scratch allocators for the CALLING
        // thread. Every thread that uses GetFrameAlloc()/GetScratchAlloc()
        // needs this to have run on it first -- Initialize() does it for
        // the calling thread automatically; Jobs worker threads get it
        // for free via the worker start/stop hooks Engine::Initialize()
        // wires into Jobs::Initialize(). Call directly only for a thread
        // that's neither of those.
        void InitializeThreadLocal();
        void ShutdownThreadLocal();

        // Advances the frame counter GetFrameAlloc()/GetScratchAlloc() use
        // to lazily reset each thread's own allocators the first time that
        // thread touches them in the new frame -- see those functions.
        // Call once per frame, from whichever thread drives the frame loop.
        void BeginFrame();

        // Preferred access: resolves to the calling thread's own frame/
        // scratch allocator (each thread has its own -- see
        // InitializeThreadLocal), resetting it first if this is that
        // thread's first touch since the last BeginFrame(). No capture
        // needed to call from inside a Jobs task lambda -- these are
        // functions, not local variables, so a lambda with no capture list
        // at all can still call them.
        [[nodiscard]] FrameAllocator&   GetFrameAlloc();
        [[nodiscard]] ScratchAllocator& GetScratchAlloc();
    } // namespace Memory
}
