#pragma once

#include <PhxRhi/PhxRhi_Types.h>
#include <vk_mem_alloc.h>

#include <volk.h>

namespace phx::rhi::vulkan
{
    class SlotAllocator
    {
    public:
        void Initialize(uint32_t max_slots)
        {
            m_max_slots = max_slots;
            m_next_new_index = 0;
        }

        rhi::DescriptorIndex AllocateSlot()
        {
            if (!m_free_list.empty()) 
            {
                auto idx = m_free_list.back();
                m_free_list.pop_back();

                return idx;
            }

            if (m_next_new_index < m_max_slots) 
            {
                return m_next_new_index++;
            }

            return rhi::cInvalidDescriptorIndex;
        }

        void FreeSlot(rhi::DescriptorIndex index)
        {
            m_free_list.push_back(index);
        }

    private:
        uint32_t m_max_slots;
        uint32_t m_next_new_index;
        std::vector<uint32_t> m_free_list;
    };

    enum class HeapType
    {
        Resource = 0,
        Sampler,
        Count
    };

	class DescriptorHeap
	{
	public:
		void Initialize(
            VkDevice vk_device,
            VmaAllocator vma_allocator,
            VkPhysicalDevice physical_device,
            HeapType heap_type,
            uint32_t max_slots);

		void Shutdown();

		rhi::DescriptorIndex Allocate(const VkDescriptorGetInfoEXT& descriptor_info);
		void Free(uint32_t index);

		VkDeviceAddress GetBufferAddress() const { return m_buffer_address; };

	private:
        size_t                  m_descriptor_stride;

        VkDevice                m_vk_device;
        VmaAllocator            m_vma_allocator;

        VkBuffer		        m_vk_buffer;
        VmaAllocation           m_vma_allocation;

        char*                   m_mapped_ptr;
        VkDeviceAddress         m_buffer_address;

        SlotAllocator           m_slot_allocator;
	};
}