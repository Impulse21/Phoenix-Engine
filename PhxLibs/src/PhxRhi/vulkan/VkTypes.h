#pragma once

#include <VkCommon.h>

#define DEFINE_ALIGNED(name, alignemnt) alignas(alignemnt) name
namespace phx::rhi::vk
{
	constexpr size_t kCacheLineSize = 64ull;
	
	struct DEFINE_ALIGNED(Buffer_VK, kCacheLineSize)
	{
		// -- 8-byte members ---
		VkBuffer		vk_buffer;
		VmaAllocation	allocation;

		VkDeviceAddress gpu_address= 0;
		void*			mapped_data = nullptr;

		// -- 4-byte members ---
		uint32_t        mapped_data_size;
		DescriptorIndex srv_index;
		DescriptorIndex UavIndex;

		// --- bitfield for booleans (1 byte) ---
		bool            SrvIsTyped : 1;
		bool            UavIsTyped : 1;

		// -- Manual Padding ---
		std::byte padding[19];
	};

	static_assert(sizeof(Buffer_VK) == kCacheLineSize, "Buffer_VK must be exactly one cache line in size!");
}