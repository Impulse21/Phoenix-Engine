#pragma once

#include <mutex>

namespace phx
{
    class VirtualMemoryArena
    {
    public:
        struct Descriptor
        {
            usize reserved_size = 32_GB;
            usize init_commit_size = 0;
        };

    public:
        PHX_NO_COPY_NO_MOVE(VirtualMemoryArena);

        VirtualMemoryArena() = default;
        ~VirtualMemoryArena() { Shutdown(); }

    public:
        void Initialize(const Descriptor& desc);
        void Shutdown();

        [[nodiscard]] void* Alloc(usize size);

        // Carve() and Commit() are both synchronized here (not in
        // LinearAllocator) -- multiple per-thread LinearAllocator instances
        // (see Memory::g_Frame / g_Scratch, thread_local) share this one
        // arena, each Carve()-ing its own region at thread start-up and
        // Commit()-ing into it as it grows. Both are rare/cold paths (once
        // per thread, or once per page-growth boundary), so a plain mutex
        // is simplest -- LinearAllocator's own hot Alloc() path never takes
        // it and stays lock-free.
        [[nodiscard]] void* Carve(usize reserve_size);
        bool Commit(void* ptr, usize size);

        void Reset();

        [[nodiscard]] void*  Base()             const { return m_base;      }
        [[nodiscard]] usize  ReservedBytes()    const { return m_reserved;  }
        [[nodiscard]] usize  CommittedBytes()   const { std::scoped_lock lock(m_mutex); return m_committed; }
        [[nodiscard]] usize  GetPageSize()      const { return m_page_size; }

    private:
        void* m_base      = nullptr;
        usize m_reserved  = 0;
        usize m_committed = 0;
        usize m_offset    = 0;
        usize m_page_size = 0;
        mutable std::mutex m_mutex;
    };
}