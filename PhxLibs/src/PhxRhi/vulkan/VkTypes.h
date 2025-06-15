#pragma once

#include "VkCommon.h"

#define RHI_DEFINE_ALIGNED(name, alignemnt) alignas(alignemnt) name
namespace phx::rhi::vk
{
	constexpr size_t kCacheLineSize = 64ull;
	
	struct RHI_DEFINE_ALIGNED(Buffer_VK, kCacheLineSize)
	{
		// -- 8-byte members ---
		VkBuffer		vk_buffer;
		VmaAllocation	allocation;

		VkDeviceAddress gpu_address= 0;
		void*			mapped_data = nullptr;
		VkBufferView	buffer_view = VK_NULL_HANDLE;

		// -- 4-byte members ---
		uint32_t        mapped_data_size = 0;
		DescriptorIndex srv_index = cInvalidDescriptorIndex;
		DescriptorIndex uav_index = cInvalidDescriptorIndex;

		// --- bitfield for booleans (1 byte) ---
		bool            srv_is_typed : 1 = false;
		bool            uav_is_typed : 1 = false;

		// -- Manual Padding ---
		std::byte padding[11];
	};

	static_assert(sizeof(Buffer_VK) == kCacheLineSize, "Buffer_VK must be exactly one cache line in size!");
}