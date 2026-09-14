#include "BumpAllocator.h"

using namespace phx::rhi;



GpuCpuRange<byte> phx::rhi::GpuBumpAllocator::Alloc(u64 size) noexcept
{
    PHX_ASSERT(size != 0);
    if (size > m_arena.size - m_offset)
        return {};

    const GpuCpuRange<byte> allocation = {
        .cpu = OffsetPointer(m_arena.cpu, m_offset),
        .gpu = OffsetPointer(m_arena.gpu, m_offset),
        .size = size
    };
    
    const u64 remaining = m_arena.size - m_offset;
    const u64 aligned_size = (size + k_alignment - 1) & ~(k_alignment - 1);
    m_offset += aligned_size < remaining ? aligned_size : remaining;
    
    return allocation;
}
