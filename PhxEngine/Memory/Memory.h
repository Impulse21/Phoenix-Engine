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
        inline VirtualMemoryArena   g_Arena;
        inline TlsfHeapAllocator    g_Heap;
        inline FrameAllocator       g_Frame;
        inline ScratchAllocator     g_Scratch;

        enum class ArenaType { Virtual, };
        void Initialize();
        void Shutdown();
        
        void BeginFrame();
    } // namespace Memory
}