#include "Memory.h"

#include <PhxEngine/Core/CVar.h>

#include "VirtualMemoryArena.h"
#include "TlsfHeapAllocator.h"
#include "FrameAllocator.h"
#include "ScratchAllocator.h"

PHX_CVAR_INT(mem_arena_size,    16,     "Virtual Arena Size GB" );
PHX_CVAR_INT(mem_heap_size,     2048,   "Heap Size MB");
PHX_CVAR_INT(mem_frame_size,    64,     "Heap Size MB");
PHX_CVAR_INT(mem_scratch_size,  32,     "Heap Size MB");

namespace phx::Memory
{
    VirtualMemoryArena   Arena;
    TlsfHeapAllocator    Heap;
    FrameAllocator       Frame;
    ScratchAllocator     Scratch;

    void Initialize()
    {
        VirtualMemoryArena::Descriptor arena_desc = {
            .reserved_size = PhxGB((usize)CVar_mem_arena_size.Get())
        };

        Arena.Initialize(arena_desc);

        const usize heap_size = PhxMB((usize)CVar_mem_heap_size.Get());
        void* heap_chunk = Arena.Alloc(heap_size);
        Heap.Initialize(heap_chunk, heap_size);


        const usize frame_size = PhxMB((usize)CVar_mem_frame_size.Get());
        void* frame_chunk = Arena.Carve(frame_size);
        Frame.Initialize(frame_chunk, frame_size);


        const usize scratch_size = PhxMB((usize)CVar_mem_scratch_size.Get());
        void* scratch_chunk = Arena.Carve(scratch_size);
        Scratch.Initialize(scratch_chunk, scratch_size);
    }

    void Shutdown()
    {
        Scratch.Shutdown();
        Frame.Shutdown();
        Heap.Shutdown();
        Arena.Shutdown();
    }
}