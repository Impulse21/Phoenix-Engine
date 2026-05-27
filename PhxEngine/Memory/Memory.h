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
        struct Desc
        {
            ArenaType arena_type                        = ArenaType::Virtual;
            VirtualMemoryArena::Descriptor arena_desc   = {};

            usize heap_size     = 2_GB;
            usize frame_size    = 64_MB;
            usize scratch_size  = 32_MB;
        };

        void Initialize(const Desc& desc);
        void Shutdown();
        
    } // namespace Memory
}