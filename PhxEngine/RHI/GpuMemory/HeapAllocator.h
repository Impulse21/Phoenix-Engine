#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/VirtualHeapCore.h>

namespace phx::rhi
{
    template<typename T>
    struct HeapAllocation
    {
        GpuCpuRange<T> range = {};
        VmaVirtualAllocation vma_alloc = nullptr;
    };

    // General-purpose sub-allocator over a GpuHeap range
    // (VirtualHeapCore + GpuCpuRange).
    //
    // Free() defers releasing the range until the GPU is done with the frame.
    // The deferred callback captures `this`: the allocator is non-movable and
    // must outlive rhi::Shutdown()'s deferred-queue flush. Main-thread only.
    class GpuHeapAllocator
    {
    public:
        static constexpr u64 k_alignment = 16;

    public:
        PHX_NO_COPY_NO_MOVE(GpuHeapAllocator);
        GpuHeapAllocator() = default;
        ~GpuHeapAllocator() { Shutdown(); }

    public:
        void Initialize(GpuCpuRange<byte> block) noexcept;

        // Releases every allocation at once. Deferred frees still in flight
        // become no-ops.
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
            FreeDeferred(allocation.vma_alloc);
        }

        // Releases every allocation at once. Asserts that no deferred frees
        // are still pending: they would otherwise free into the reset block.
        void Reset() noexcept;

    private:
        void FreeDeferred(VmaVirtualAllocation vma_alloc) noexcept;

        VirtualHeapCore   m_core;
        GpuCpuRange<byte> m_storage;
        u32               m_pending_frees = 0;
    };
}
