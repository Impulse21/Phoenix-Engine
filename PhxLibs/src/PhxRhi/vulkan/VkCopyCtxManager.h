#pragma once

#include <mutex>

#include "VkCommon.h"

namespace phx::rhi::vk
{
	class VkGfxDeviceImpl;

	struct CopyCtx
	{
		VkCommandPool transfer_command_pool = VK_NULL_HANDLE;
		VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
		VkCommandPool transition_command_pool = VK_NULL_HANDLE;
		VkCommandBuffer transition_command_buffer = VK_NULL_HANDLE;

		VkFence fence = VK_NULL_HANDLE;
		VkSemaphore semaphore = VK_NULL_HANDLE;

		GpuBufferHandle upload_buffer;
		constexpr bool IsValid() const { return transfer_command_buffer != VK_NULL_HANDLE; }
	};

	struct CopyCtxManager
	{
		VkGfxDeviceImpl* gfx_device;
		std::mutex lock;
		std::vector<CopyCtx> free_list;

		void Initialize(VkGfxDeviceImpl* device);
		void Shutdown();
		CopyCtx Allocate(uint64_t staging_size);

		void SubmitAndWait(CopyCtx copy_ctx);
	};
}