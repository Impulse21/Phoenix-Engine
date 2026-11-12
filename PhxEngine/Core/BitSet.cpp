#include "BitSet.h"

#include <PhxEngine/Memory/LinearAllocator.h>


phx::BitSet phx::BitSet::Create(LinearAllocator& linear_allocator, u32 num_bits) noexcept
{
    BitSet bitset;
    bitset.m_words = linear_allocator.Alloc<u64>(num_bits / 64 + 1);
    bitset.num_bits = num_bits;

    return bitset;
}
