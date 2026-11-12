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

        }

        bool Get(u32 index) const noexcept
        {

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