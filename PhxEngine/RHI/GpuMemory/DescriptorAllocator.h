#pragma once

#include <PhxEngine/Memory/SlotCore.h>
#include <PhxEngine/Core/Thread.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <concepts>

namespace phx::rhi
{
    // Hands out bindless descriptor slots in a descriptor heap range
    // (SlotCore + GpuCpuRange).
    //
    // Free() defers returning the slot until the GPU is done with the frame.
    // The deferred callback captures `this`: the allocator is non-movable and
    // must outlive rhi::Shutdown()'s deferred-queue flush. Main-thread only
    // (SlotCore is unsynchronized; deferred callbacks run on the main thread).
    class DescriptorAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(DescriptorAllocator);
        DescriptorAllocator() = default;

        void Initialize(GpuCpuRange<byte> block, u64 descriptor_size) noexcept
        {
            m_storage = block;
            m_descriptor_size = descriptor_size;
            m_slots.Initialize(static_cast<DescriptorIndex>(block.size / descriptor_size));
        }

        [[nodiscard]] DescriptorIndex Allocate(TextureHandle handle) noexcept
        {
            PHX_ASSERT(Thread::IsMainThread());
            const DescriptorIndex slot = m_slots.Allocate();
            
            PHX_ASSERT(slot != kInvalidDescriptorIndex);
            if (slot == kInvalidDescriptorIndex)
                return kInvalidDescriptorIndex;

            void* dest = OffsetPointer(m_storage.cpu, static_cast<u64>(slot) * m_descriptor_size);
            rhi::WriteDescriptor(handle, dest);

            return slot;
        }

        void Free(DescriptorIndex index) noexcept
        {
            PHX_ASSERT(Thread::IsMainThread());
            if (index == kInvalidDescriptorIndex)
                return;

            rhi::DeferUntilGpuComplete([this, index]
            {
                m_slots.Free(index);
            });
        }

    private:
        GpuCpuRange<byte> m_storage;
        u64 m_descriptor_size = 0;
        phx::SlotCore<DescriptorIndex, kInvalidDescriptorIndex> m_slots;
    };
}
