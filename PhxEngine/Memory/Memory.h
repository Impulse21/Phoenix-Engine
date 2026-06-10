#pragma once

#include <PhxEngine/Memory/VirtualMemoryArena.h>

#include "VirtualMemoryArena.h"
#include "TlsfHeapAllocator.h"
#include "FrameAllocator.h"
#include "ScratchAllocator.h"

namespace phx
{
    namespace Memory
    {
        inline static VirtualMemoryArena   g_Arena;
        inline static TlsfHeapAllocator    g_Heap;
        inline static FrameAllocator       g_Frame;
        inline static ScratchAllocator     g_Scratch;

        enum class ArenaType { Virtual, };
        void Initialize();
        void Shutdown();
        
    } // namespace Memory
}