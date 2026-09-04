#include "TlsfHeapAllocator.h"

#include "tlsf/tlsf.h"

namespace phx
{
    void TlsfHeapAllocator::Initialize(void* block, usize block_size)
    {
        m_tlsf = tlsf_create_with_pool(block, block_size);
        m_block_size = block_size;
    }

    void TlsfHeapAllocator::Shutdown()
    {
        if (m_tlsf)
        {
            tlsf_destroy(m_tlsf);
            m_tlsf = nullptr;
            m_block_size = 0;
        }
    }

    void* TlsfHeapAllocator::Alloc(usize size, usize alignment)
    {
        return tlsf_memalign(m_tlsf, alignment, size);
    }

    void TlsfHeapAllocator::Free(void* ptr)
    {
        tlsf_free(m_tlsf, ptr);
    }
}