#include "Memory.h"

#include <PhxEngine/Core/CVar.h>

PHX_CVAR_INT(mem_arena_size,    16,     "Virtual Arena Size GB" );
PHX_CVAR_INT(mem_heap_size,     2048,   "Heap Size MB");
PHX_CVAR_INT(mem_frame_size,    64,     "Heap Size MB");
PHX_CVAR_INT(mem_scratch_size,  32,     "Heap Size MB");

namespace phx::Memory
{
    void Initialize()
    {
        VirtualMemoryArena::Descriptor arena_desc = {
            .reserved_size = PhxGB((usize)CVar_mem_arena_size.Get())
        };

        g_Arena.Initialize(arena_desc);

        const usize heap_size = PhxMB((usize)CVar_mem_heap_size.Get());
        void* heap_chunk = g_Arena.Alloc(heap_size);
        g_Heap.Initialize(heap_chunk, heap_size);


        const usize frame_size = PhxMB((usize)CVar_mem_frame_size.Get());
        void* frame_chunk = g_Arena.Carve(frame_size);
        g_Frame.Initialize(frame_chunk, frame_size);


        const usize scratch_size = PhxMB((usize)CVar_mem_scratch_size.Get());
        void* scratch_chunk = g_Arena.Carve(scratch_size);
        g_Scratch.Initialize(scratch_chunk, scratch_size);
    }

    void Shutdown()
    {
        g_Scratch.Shutdown();
        g_Frame.Shutdown();
        g_Heap.Shutdown();
        g_Arena.Shutdown();
    }

    void BeginFrame()
    {
        g_Scratch.Reset();
        g_Frame.Reset();
    }
}  // namespace phx::Memory