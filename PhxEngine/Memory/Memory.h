#pragma once

#include <PhxEngine/Memory/VirtualMemoryArena.h>

#include "VirtualMemoryArena.h"
#include "FrameAllocator.h"
#include "ScratchAllocator.h"

namespace phx
{
    namespace Memory
    {
        extern VirtualMemoryArena g_Arena;

        enum class ArenaType { Virtual, };
        
        void Initialize();
        void Shutdown();

        void InitializeThreadLocal();
        void ShutdownThreadLocal();

        void BeginFrame();

        [[nodiscard]] FrameAllocator&   GetFrameAlloc();
        [[nodiscard]] ScratchAllocator& GetScratchAlloc();
    } // namespace Memory
}
