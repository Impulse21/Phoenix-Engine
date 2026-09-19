#pragma once

#include <PhxEngine/Core/SlotAllocator.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <concepts>

namespace phx::rhi
{
    class DescriptorAllocator
    {
    public:
        DescriptorAllocator() = default;

        void Initialize(GpuCpuRange<byte> block, u64 descriptor_size) noexcept
        {
            m_storage = block;
            m_descriptor_size = descriptor_size;
            m_slot_allocator.Initialize(static_cast<u32>(block.size / descriptor_size));
        }

        [[nodiscard]] DescriptorIndex Allocate(TextureHandle handle) noexcept
        {
            const DescriptorIndex slot = m_slot_allocator.AllocateSlot();
            if (slot == kInvalidDescriptorIndex)
                return kInvalidDescriptorIndex;

            void* dest = OffsetPointer(m_storage.cpu, static_cast<u64>(slot) * m_descriptor_size);
            rhi::WriteDescriptor(handle, dest);

            return slot;
        }

        void Free(DescriptorIndex index) noexcept
        {
            if (index == kInvalidDescriptorIndex)
                return;
                
            rhi::DeferUntilGpuComplete([this, index]
            {
                m_slot_allocator.FreeSlot(index);
            });
        }

    private:
        GpuCpuRange<byte> m_storage;
        u64 m_descriptor_size = 0;
        phx::SlotAllocator<DescriptorIndex> m_slot_allocator;
    };
}
