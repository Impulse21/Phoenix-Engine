#pragma once

#include <PhxEngine/Core/StaticArray.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>

namespace phx::renderer
{
    // Per-frame GPU-visible bump memory: one GpuBumpAllocator per frame in
    // flight, carved from a single CPU-visible range.
    //
    // A slot is only reused after rhi::BeginFrame() has waited for the GPU to
    // finish the frame that last used it, so BeginFrame() here can reset it
    // safely. Call it once per frame after rhi::BeginFrame(), before any
    // Current() use. Owned by the renderer; not thread-safe.
    class GpuFrameRing
    {
    public:
        // Slice bases are aligned to this so every slot starts as aligned as
        // the range itself (GpuBumpAllocator aligns relative to its base).
        static constexpr u64 k_slice_alignment = 256;

    public:
        void Initialize(rhi::GpuCpuRange<byte> range) noexcept
        {
            const u64 slice_size = (range.size / rhi::MaxFramesInFlight) & ~(k_slice_alignment - 1);
            PHX_ASSERT(slice_size > 0);

            for (u32 i = 0; i < rhi::MaxFramesInFlight; ++i)
                m_slots[i].Initialize(range.Slice(i * slice_size, slice_size));
        }

        // Resets and returns this frame's allocator.
        rhi::GpuBumpAllocator& BeginFrame() noexcept
        {
            rhi::GpuBumpAllocator& slot = Current();
            slot.Reset();
            return slot;
        }

        [[nodiscard]] rhi::GpuBumpAllocator& Current() noexcept
        {
            const u64 index = rhi::GetFrameIndex();
            PHX_ASSERT(index < m_slots.Size());
            return m_slots[index];
        }

    private:
        StaticArray<rhi::GpuBumpAllocator, rhi::MaxFramesInFlight> m_slots;
    };
}
