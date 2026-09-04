#include "Memory.h"

#include <PhxEngine/Core/Log.h>

#include <PhxEngine/Core/CVar.h>

#include <atomic>

PHX_CVAR_INT(mem_arena_size,    16,     "Virtual Arena Size GB" );
PHX_CVAR_INT(mem_heap_size,     2048,   "Heap Size MB");
PHX_CVAR_INT(mem_frame_size,    64,     "Per-thread Frame allocator size MB");
PHX_CVAR_INT(mem_scratch_size,  32,     "Per-thread Scratch allocator size MB");

namespace
{
    // One instance per thread -- Memory::InitializeThreadLocal() carves
    // each thread its own region out of Memory::g_Arena, so these never
    // need locking (see LinearAllocator; the only shared state is
    // VirtualMemoryArena's own Carve()/Commit(), synchronized there).
    thread_local phx::FrameAllocator   t_frame;
    thread_local phx::ScratchAllocator t_scratch;
    thread_local bool                  t_initialized     = false;
    thread_local u64                   t_last_reset_frame = 0;

    // Shared: bumped once per frame by BeginFrame(). Each thread compares
    // this against its own t_last_reset_frame to know whether it still
    // needs to reset for the current frame -- lazily, the first time it
    // actually touches its allocator, since a Jobs worker may go several
    // frames without running anything and Taskflow gives no way to reach
    // into a specific worker's thread-local state from the outside.
    std::atomic<u64> g_frame_number{ 0 };

    void EnsureResetForCurrentFrame()
    {
        const u64 current = g_frame_number.load(std::memory_order_relaxed);
        if (t_last_reset_frame != current)
        {
            t_frame.Reset();
            t_scratch.Reset();
            t_last_reset_frame = current;
        }
    }
}

namespace phx::Memory
{
    void Initialize()
    {
        PHX_LOG_INFO(Log::Channels::Memory, "Initializing Memory System: Arena Size = {0} GB, Heap Size = {1} MB, Frame Reserved Size = {2} MB/thread, Scratch Reserved Size = {3} MB/thread",
            CVar_mem_arena_size.Get(),
            CVar_mem_heap_size.Get(),
            CVar_mem_frame_size.Get(),
            CVar_mem_scratch_size.Get());

        VirtualMemoryArena::Descriptor arena_desc = {
            .reserved_size = PhxGB((usize)CVar_mem_arena_size.Get())
        };

        g_Arena.Initialize(arena_desc);

        InitializeThreadLocal(); // this (the calling) thread's own Frame/Scratch
    }

    void Shutdown()
    {
        ShutdownThreadLocal();
        g_Arena.Shutdown();
    }

    void InitializeThreadLocal()
    {
        PHX_ASSERT(!t_initialized && "Memory::InitializeThreadLocal called twice on the same thread");

        const usize frame_size = PhxMB((usize)CVar_mem_frame_size.Get());
        void* frame_chunk = g_Arena.Carve(frame_size);
        t_frame.Initialize(&g_Arena, frame_chunk, frame_size);

        const usize scratch_size = PhxMB((usize)CVar_mem_scratch_size.Get());
        void* scratch_chunk = g_Arena.Carve(scratch_size);
        t_scratch.Initialize(&g_Arena, scratch_chunk, scratch_size);

        t_last_reset_frame = g_frame_number.load(std::memory_order_relaxed);
        t_initialized = true;
    }

    void ShutdownThreadLocal()
    {
        if (!t_initialized)
            return;

        t_scratch.Shutdown();
        t_frame.Shutdown();
        t_initialized = false;
    }

    void BeginFrame()
    {
        g_frame_number.fetch_add(1, std::memory_order_relaxed);
    }

    FrameAllocator& GetFrameAlloc()
    {
        PHX_ASSERT(t_initialized && "Memory::GetFrameAlloc() called on a thread that never called Memory::InitializeThreadLocal()");
        EnsureResetForCurrentFrame();
        return t_frame;
    }

    ScratchAllocator& GetScratchAlloc()
    {
        PHX_ASSERT(t_initialized && "Memory::GetScratchAlloc() called on a thread that never called Memory::InitializeThreadLocal()");
        EnsureResetForCurrentFrame();
        return t_scratch;
    }
}  // namespace phx::Memory
