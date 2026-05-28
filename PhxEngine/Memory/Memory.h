#pragma once

#include <PhxEngine/Memory/VirtualMemoryArena.h>

namespace phx
{
    class TlsfHeapAllocator;
    class FrameAllocator;
    class ScratchAllocator;

    namespace Memory
    {
        extern VirtualMemoryArena   Arena;
        extern TlsfHeapAllocator    Heap;
        extern FrameAllocator       Frame;
        extern ScratchAllocator     Scratch;

        enum class ArenaType { Virtual, };
        void Initialize();
        void Shutdown();
        
    } // namespace Memory
}