#pragma once

#include "VulkanTypes.h"

#include <volk.h>

namespace phx::rhi
{
    class VulkanBackend;
    class VulkanGpuAllocator;
}

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
		void Initialize(phx::rhi::VulkanBackend* vulkan_backend, VulkanGpuAllocator* vulkan_allocator, HeapType heap_type, uint32_t max_slots);
		void Shutdown();


		rhi::DescriptorIndex Allocate(const VkDescriptorGetInfoEXT& descriptor_info);
		void Free(uint32_t index);

		VkDeviceAddress GetBufferAddress() const { return m_buffer_address; };

	private:
		VulkanBackend*          m_vulkan_backend;
        VulkanGpuAllocator*     m_vulkan_allocator;
		VkDescriptorType        m_descriptor_type;
        size_t                  m_descriptor_stride;

        VkBuffer		        m_vk_buffer;
        VmaAllocation	        m_vma_allocation;

        char*                   m_mapped_ptr;
        VkDeviceAddress         m_buffer_address;

        SlotAllocator           m_allocator;
	};
}