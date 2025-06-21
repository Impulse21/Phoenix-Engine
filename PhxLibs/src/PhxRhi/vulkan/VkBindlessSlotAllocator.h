#pragma once

#include <vector>
#include <PhxRhi/RHICommon.h>
namespace phx::RHI::vk
{
    class BindlessSlotAllocator
    {
    public:
        void Initialize(uint32_t max_slots)
        {
            m_max_slots = max_slots;
            m_free_list.reserve(max_slots);
            for (uint32_t i = 0; i < max_slots; ++i)
            {
                m_free_list.push_back(max_slots - 1 - i);
            }
        }

        RHI::DescriptorIndex AllocateSlot()
        {
            if (m_free_list.empty())
                return RHI::cInvalidDescriptorIndex;

            RHI::DescriptorIndex out_index = m_free_list.back();
            m_free_list.pop_back();

            return out_index;
        }

        void FreeSlot(RHI::DescriptorIndex index)
        {
            if (index < m_max_slots)
            {
                m_free_list.push_back(index);
            }
        }

    private:
        uint32_t m_max_slots;
        std::vector<uint32_t> m_free_list;
    };
}