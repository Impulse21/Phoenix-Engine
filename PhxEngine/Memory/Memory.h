#pragma once

#include <PhxEngine/Memory/VirtualPageSource.h>
#include <PhxEngine/Memory/FrameArena.h>
#include <PhxEngine/Memory/FrameAllocator.h>
#include <PhxEngine/Memory/ScratchAllocator.h>

namespace phx
{
    namespace Memory
    {
        void Initialize();

        // Must run after every thread that used frame/scratch memory has
        // exited (i.e. after Jobs::Shutdown); shuts down the calling thread's
        // allocators.
        void Shutdown();

        // Frame boundary. Releases this frame's frame memory (FrameArena,
        // kFrameLatency = 1). Call once per frame after every job that
        // allocated or reads frame memory has finished. Detached jobs must
        // not use frame memory across it.
        void EndFrame();

        // The calling thread's allocators, created on first use on any
        // thread (no per-thread init). Don't pass them to other threads.
        [[nodiscard]] FrameAllocator&   GetFrameAlloc();
        [[nodiscard]] ScratchAllocator& GetScratchAlloc();

        // Global source; exposed for stats/diagnostics.
        [[nodiscard]] VirtualPageSource& GetPageSource();
    } // namespace Memory
}
