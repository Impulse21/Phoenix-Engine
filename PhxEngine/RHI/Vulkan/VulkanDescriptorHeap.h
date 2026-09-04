#pragma once


#include <vk_mem_alloc.h>

#include <volk.h>

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Core/SlotAllocator.h>

namespace phx::rhi::vulkan
{
    enum class HeapType
    {
        Resource = 0,
        Sampler,
        Count
    };

	class DescriptorHeap
	{
        using DescriptorAllocator = phx::SlotAllocator<rhi::DescriptorIndex, rhi::kInvalidDescriptorIndex>;

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

        DescriptorAllocator     m_slot_allocator;
	};
}