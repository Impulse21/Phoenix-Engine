#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::rhi
{
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
            m_offset = 0;
        }

        [[nodiscard]] GpuCpuRange<byte> Alloc(u64 size) noexcept;

        template<typename T>
        [[nodiscard]] GpuCpuRange<T> Alloc(u64 num_elements) noexcept
        {
            static_assert(alignof(T) <= k_alignment);
            const GpuCpuRange<byte> allocation = Alloc(num_elements * sizeof(T));

            return {
                .cpu = reinterpret_cast<T*>(allocation.cpu),
                .gpu = reinterpret_cast<T*>(allocation.gpu),
                .size = allocation.size};
        }

        u64 Used() const { return m_offset; }
        u64 Free() const { return m_arena.size - m_offset; }

        u64 Capacity() const { return m_arena.size; }

        void Reset() noexcept
        {
            m_offset = 0;
        }

    private:
        GpuCpuRange<byte> m_arena;
        alignas(8) u64 m_offset;

    };
}