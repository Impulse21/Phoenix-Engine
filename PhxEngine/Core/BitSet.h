#pragma once

#include "PhxDefines.h"

namespace phx
{
    class LinearAllocator;

    class BitSet
    {
    public:
        // TODO: this works with scratch or frame allocator - would be good to make this more generic.
        // the tricky part is indicating if we need to free the memory or not. For now just use linear allocator
        // and don't free memory.
        static BitSet Create(LinearAllocator& linear_allocator, u32 num_bits) noexcept;

    public:
        BitSet() = default;
        BitSet(const BitSet&) = default;
        BitSet(BitSet&&) = default;
        BitSet& operator=(const BitSet&) = default;
        BitSet& operator=(BitSet&&) = default;

        void Set(u32 index, bool value) noexcept
        {
            PHX_ASSERT(index < num_bits);
            const u64 mask = u64(1) << (index & 63);
            if (value)
                m_words[index >> 6] |= mask;
            else
                m_words[index >> 6] &= ~mask;
        }

        bool Get(u32 index) const noexcept
        {
            PHX_ASSERT(index < num_bits);
            return (m_words[index >> 6] >> (index & 63)) & 1;
        }

    private:
        u32     num_bits = 0;
        u64*    m_words = nullptr;
    };

    template <u32 N>
    class FixedBitSet
    {
    public:
        FixedBitSet() = default;
        FixedBitSet(const FixedBitSet&) = default;
        FixedBitSet(FixedBitSet&&) = default;
        FixedBitSet& operator=(const FixedBitSet&) = default;
        FixedBitSet& operator=(FixedBitSet&&) = default;

        void Set(u32 index, bool value) noexcept;
        bool Get(u32 index) const noexcept;

    private:
        u64 m_words[N / 64 + 1];
    };
}