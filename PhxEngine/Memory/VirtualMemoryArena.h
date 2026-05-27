#pragma once

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
        [[nodiscard]] void* Carve(usize reserve_size);

        bool Commit(void* ptr, usize size);
        
        void Reset();

        [[nodiscard]] void*  Base          () const { return m_base;      }
        [[nodiscard]] usize  ReservedBytes () const { return m_reserved;  }
        [[nodiscard]] usize  CommittedBytes() const { return m_committed; }

    private:
        void* m_base      = nullptr;
        usize m_reserved  = 0;
        usize m_committed = 0;
        usize m_offset    = 0;
        usize m_page_size = 0;
    };
}