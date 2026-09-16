#pragma once

#include <PhxEngine/RHI/RHITypes.h>

#include <vk_mem_alloc.h>

namespace phx::rhi
{
    template<typename T>
    struct HeapAllocation
    {
        GpuCpuRange<T> range = {};
        VmaVirtualAllocation vma_alloc = nullptr;
    };

    class GpuHeapAllocator
    {
    public:
        static constexpr u64 k_alignment = 16;

    public:
        PHX_MOVE_ONLY(GpuHeapAllocator);
        GpuHeapAllocator() = default;
        ~GpuHeapAllocator() { Shutdown(); }

    public:
        void Initialize(GpuCpuRange<byte> block) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] HeapAllocation<byte> Alloc(u64 size) noexcept;

        template<typename T>
        [[nodiscard]] HeapAllocation<T> Alloc(u64 num_elements) noexcept
        {
            static_assert(alignof(T) <= k_alignment);
            const HeapAllocation<byte> allocation = Alloc(num_elements * sizeof(T));

            return {
                .range = {
                    .cpu = reinterpret_cast<T*>(allocation.range.cpu),
                    .gpu = reinterpret_cast<T*>(allocation.range.gpu),
                    .size = allocation.range.size,
                },
                .vma_alloc = allocation.vma_alloc,
            };
        }

        template<typename T>
        void Free(HeapAllocation<T> allocation) noexcept
        {
            vmaVirtualFree(m_virtual_block, allocation.vma_alloc);
        }

        void Reset() noexcept;

    private:
        VmaVirtualBlock m_virtual_block = nullptr;
        GpuCpuRange<byte> m_storage;
    };
}