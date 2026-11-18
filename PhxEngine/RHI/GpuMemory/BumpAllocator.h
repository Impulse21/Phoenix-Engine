#pragma once

#include <PhxEngine/Memory/BumpCore.h>
#include <PhxEngine/RHI/RHITypes.h>

namespace phx::rhi
{
    // Bump allocation over a CPU-visible GPU range (BumpCore + GpuCpuRange).
    // Each allocation is aligned relative to the range base; the base itself
    // comes from a GpuHeap and is assumed at least k_alignment-aligned.
    class GpuBumpAllocator
    {
    public:

        static constexpr u64 k_alignment = 16;

        PHX_MOVE_ONLY(GpuBumpAllocator);
        GpuBumpAllocator() = default;

    public:
        void Initialize(GpuCpuRange<byte> block) noexcept
        {
            m_arena = block;
            m_core.Initialize(block.size);
        }

        // Returns an invalid range if it doesn't fit.
        [[nodiscard]] GpuCpuRange<byte> Alloc(u64 size, u64 alignment = k_alignment) noexcept;

        // `size` of the result is in bytes (num_elements * sizeof(T)).
        template<typename T>
        [[nodiscard]] GpuCpuRange<T> Alloc(u64 num_elements = 1, u64 alignment = k_alignment) noexcept
        {
            PHX_ASSERT(alignof(T) <= alignment);
            const GpuCpuRange<byte> allocation = Alloc(num_elements * sizeof(T), alignment);

            return {
                .cpu = reinterpret_cast<T*>(allocation.cpu),
                .gpu = reinterpret_cast<T*>(allocation.gpu),
                .size = allocation.size};
        }

        u64 Used() const { return m_core.Used(); }
        u64 Free() const { return m_core.Remaining(); }

        u64 Capacity() const { return m_core.Capacity(); }

        void Reset() noexcept { m_core.Reset(); }

    private:
        GpuCpuRange<byte> m_arena = {};
        BumpCore m_core;
    };
}
