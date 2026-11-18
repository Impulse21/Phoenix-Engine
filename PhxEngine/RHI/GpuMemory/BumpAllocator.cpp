#include "BumpAllocator.h"

using namespace phx::rhi;

GpuCpuRange<byte> phx::rhi::GpuBumpAllocator::Alloc(u64 size, u64 alignment /* = k_alignment*/) noexcept
{
    PHX_ASSERT(size != 0);

    const u64 offset = m_core.Alloc(size, alignment);
    if (offset == BumpCore::kInvalidOffset)
        return {};

    return {
        .cpu  = OffsetPointer(m_arena.cpu, offset),
        .gpu  = OffsetPointer(m_arena.gpu, offset),
        .size = size,
    };
}
