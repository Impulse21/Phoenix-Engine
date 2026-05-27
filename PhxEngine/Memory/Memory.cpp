#include "Memory.h"

#include "VirtualMemoryArena.h"
#include "TlsfHeapAllocator.h"
#include "FrameAllocator.h"
#include "ScratchAllocator.h"

namespace phx::Memory
{
    VirtualMemoryArena   Arena;
    TlsfHeapAllocator    Heap;
    FrameAllocator       Frame;
    ScratchAllocator     Scratch;

    void Initialize(const Desc& desc)
    {
        Arena.Initialize(desc.arena_desc);

        void* heap_chunk = Arena.Alloc(desc.heap_size);
        Heap.Initialize(heap_chunk, desc.heap_size);

        void* frame_chunk = Arena.Carve(desc.frame_size);
        Frame.Initialize(frame_chunk, desc.frame_size);

        void* scratch_chunk = Arena.Carve(desc.scratch_size);
        Scratch.Initialize(scratch_chunk, desc.scratch_size);
    }

    void Shutdown()
    {
        Scratch.Shutdown();
        Frame.Shutdown();
        Heap.Shutdown();
        Arena.Shutdown();
    }
}