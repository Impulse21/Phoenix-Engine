#include "Memory.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CVar.h>

PHX_CVAR_INT(mem_reserve_size,  16,  "Virtual address space reserved for the page source (GB)");
PHX_CVAR_INT(mem_chunk_size,    256, "Page source chunk size (KB); frame/scratch allocators grow one chunk at a time");

namespace
{
    phx::VirtualPageSource g_page_source;
    phx::FrameArena        g_frame_arena;

    // One per thread, initialized lazily on first use. Hot-path allocation
    // never locks; only acquiring a new chunk does (see VirtualPageSource).
    //
    // Thread exit destroys them: scratch returns its chunks to the page
    // source, frame chunks stay with the FrameArena until the frame ends.
    thread_local phx::FrameAllocator   t_frame;
    thread_local phx::ScratchAllocator t_scratch;
}

namespace phx::Memory
{
    void Initialize()
    {
        const VirtualPageSource::Descriptor desc = {
            .reserve_size = PhxGB(static_cast<usize>(CVar_mem_reserve_size.Get())),
            .chunk_size   = PhxKB(static_cast<usize>(CVar_mem_chunk_size.Get())),
        };

        if (!g_page_source.Initialize(desc))
        {
            PHX_LOG_ERROR(Log::Channels::Memory, "Failed to initialize the page source");
            PHX_ASSERT(false);
            return;
        }

        g_frame_arena.Initialize(g_page_source);

        PHX_LOG_INFO(Log::Channels::Memory,
            "Initializing Memory System: reserved {0} GB, chunk size {1} KB, frame latency {2}",
            PhxBytesToGB(g_page_source.ReservedBytes()),
            PhxBytesToKB(g_page_source.ChunkSize()),
            FrameArena::kFrameLatency);
    }

    void Shutdown()
    {
        // Other threads' allocators are gone with their threads; this
        // thread's are shut down explicitly so a later Initialize() starts
        // clean.
        t_scratch.Shutdown();
        t_frame.Shutdown();

        g_frame_arena.Shutdown();
        g_page_source.Shutdown();
    }

    void EndFrame()
    {
        g_frame_arena.EndFrame();
    }

    FrameAllocator& GetFrameAlloc()
    {
        PHX_ASSERT(g_frame_arena.IsInitialized() && "Memory::GetFrameAlloc() before Memory::Initialize()");
        if (!t_frame.IsInitialized())
        {
            t_frame.Initialize(g_frame_arena);
            t_frame.BindToCurrentThread();
        }
        return t_frame;
    }

    ScratchAllocator& GetScratchAlloc()
    {
        PHX_ASSERT(g_page_source.IsInitialized() && "Memory::GetScratchAlloc() before Memory::Initialize()");
        if (!t_scratch.IsInitialized())
        {
            t_scratch.Initialize(g_page_source);
            t_scratch.BindToCurrentThread();
        }
        return t_scratch;
    }

    VirtualPageSource& GetPageSource()
    {
        return g_page_source;
    }
}  // namespace phx::Memory
