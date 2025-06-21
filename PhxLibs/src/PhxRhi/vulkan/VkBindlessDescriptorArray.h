#pragma once

#include "VkRhi_Internal.h"
#include "VkBindlessSlotAllocator.h"

namespace phx::RHI::vk
{
	class VkGfxDeviceImpl;
	class VkBindlessDescriptorArray
	{
	public:
		void Initialize(VkGfxDeviceImpl* device, VkDescriptorType descriptor_type, uint32_t max_slots);
		void Shutdown();


		RHI::DescriptorIndex Allocate(const VkDescriptorGetInfoEXT& descriptor_info);
		void Free(uint32_t index);

		VkDeviceAddress GetBufferAddress() const { return m_buffer_address; };

	private:
		VkGfxDeviceImpl* m_device;
		VkDescriptorType m_descriptor_type;

		RHI::GpuBufferHandle m_buffer;

		char* m_mapped_data;
		VkDeviceAddress m_buffer_address;
		uint32_t m_descriptor_size;

		BindlessSlotAllocator m_slot_allocator;
	};
}

