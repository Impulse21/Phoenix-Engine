#pragma once

#include <PhxEngine/Memory/VirtualMemoryArena.h>

namespace phx
{
    class LinearAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(LinearAllocator);

        LinearAllocator() = default;

    public:
        void Initialize(void* block, usize block_size);
        void Shutdown();

        [[nodiscard]] void* Alloc(usize size, usize alignment = 8);

        void Reset() { m_committed = nullptr; }

        [[nodiscard]] usize GetUsedBytes() const { return m_current - m_base; }
        [[nodiscard]] usize GetCommittedBytes() const { return m_committed - m_base; }
        [[nodiscard]] usize GetFreeBytes() const { return m_end - m_current; }
        [[nodiscard]] usize GetTotalBytes() const { return m_end - m_base; }
        
    private:
        bool GrowCommit(usize needed);

        VirtualMemoryArena* m_arena     = nullptr;
        u8*                 m_base      = nullptr;
        u8*                 m_current   = nullptr;
        u8*                 m_committed = nullptr;
        u8*                 m_end       = nullptr;
    };
}