#include "BitSet.h"

#include <PhxEngine/Memory/LinearAllocator.h>

#include <cstring>


phx::BitSet phx::BitSet::Create(LinearAllocator& linear_allocator, u32 num_bits) noexcept
{
    BitSet bitset;
    // Frame/scratch memory is recycled, so the words must start cleared.
    const u32 num_words = num_bits / 64 + 1;
    bitset.m_words = linear_allocator.Alloc<u64>(num_words);
    std::memset(bitset.m_words, 0, sizeof(u64) * num_words);
    bitset.num_bits = num_bits;

    return bitset;
}
