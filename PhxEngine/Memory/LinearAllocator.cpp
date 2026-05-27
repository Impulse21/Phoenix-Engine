#include "LinearAllocator.h"

namespace phx
{
    void LinearAllocator::Initialize(void* block, usize block_size)
    {
        PHX_UNUSED(block);
        PHX_UNUSED(block_size);
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
        PHX_UNUSED(size);
        PHX_UNUSED(alignment);
        
        return nullptr;
    }
}