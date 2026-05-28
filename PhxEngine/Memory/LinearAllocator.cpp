#include "LinearAllocator.h"

namespace phx
{
    void LinearAllocator::Initialize(void* block, usize block_size)
    {
        m_arena     = nullptr;
        m_base      = static_cast<u8*>(block);
        m_current   = m_base;
        m_committed = m_base + block_size;  // whole block already committed
        m_end       = m_base + block_size;
    }

    void LinearAllocator::Initialize(VirtualMemoryArena* arena, void* base, usize reserve_size)
    {
        PHX_ASSERT(arena != nullptr);
        m_arena     = arena;
        m_base      = static_cast<u8*>(base);
        m_current   = m_base;
        m_committed = m_base;              // nothing committed yet
        m_end       = m_base + reserve_size;
    }

    void LinearAllocator::Shutdown() 
    {
        m_arena = nullptr;
        m_base = nullptr;
        m_current = nullptr;
        m_committed = nullptr;
        m_end = nullptr;
    }

    void* LinearAllocator::Alloc(usize size, usize alignment)
    {
        // Align current pointer
        uptr addr    = reinterpret_cast<uptr>(m_current);
        uptr aligned = (addr + alignment - 1) & ~(alignment - 1);
        u8*  result  = reinterpret_cast<u8*>(aligned);
        u8*  next    = result + size;

        PHX_ASSERT(next <= m_end);

        if (next > m_committed)
        {
            if (!GrowCommit((usize)(next - m_committed)))
                return nullptr;
        }

        m_current = next;
        return result;
    }

    bool LinearAllocator::GrowCommit(usize needed)
    {
        // Should never be called in block mode
        PHX_ASSERT(m_arena != nullptr);

        const usize page_size = m_arena->GetPageSize();
        usize to_commit = (needed + page_size - 1) & ~(page_size - 1);

        // Don't exceed reservation
        usize remaining = (usize)(m_end - m_committed);
        if (to_commit > remaining)
            return false;

        if (!m_arena->Commit(m_committed, to_commit))
            return false;

        m_committed += to_commit;
        return true;
    }
}