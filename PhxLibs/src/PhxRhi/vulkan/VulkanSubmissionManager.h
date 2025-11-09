#pragma once

#include <PhxCore/StaticArray.h>
#include <PhxRhi/ISubmissionManager.h>

#include "VulkanCommandBuffer.h"
#include <volk.h>

#include <deque>

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanResourceManager;

	struct VulkanSubmissionManager : public ISubmissionManager
	{
		VulkanBackend* vulkan_backend;
		VulkanResourceManager* vulkan_resource_manager;

		struct PerThreadData
		{
			struct CommandPool
			{
				VkCommandPool vk_cmd_pool;
				std::vector<std::unique_ptr<phx::rhi::VulkanCommandBuffer>> buffer_pool;
				std::vector<phx::rhi::VulkanCommandBuffer*> free_buffers;

				phx::rhi::VulkanCommandBuffer* GetFreeBuffer();
			};

			CommandPool graphics_cmd_pool;
			CommandPool compute_cmd_pool;
			CommandPool upload_cmd_pool;

		};

		struct PendingCommandBuffer
		{
			ICommandBuffer* buffer;
			FenceHandle     fence;
		};

		size_t frame_number = 0;
		size_t num_threads;
		std::unique_ptr<PerThreadData[]> per_thread_cmd_pool;
		std::vector<PendingCommandBuffer> inflight_command_queue;

		std::mutex inglight_commands_queue_mutex;
		struct Frame
		{
			VkSemaphore present_semaphore =VK_NULL_HANDLE;
			VkSemaphore render_semaphore = VK_NULL_HANDLE;
			VkFence render_fence = VK_NULL_HANDLE;

			VkFence frame_fence = VK_NULL_HANDLE;

		};
		StaticArray<Frame, kBufferCount> frames;

		// -- Interface implementation ---
		bool Initialize() override;
		void Shutdown() override;

		void BeginFrame(SwapchainHandle swapChain) override;
		void EndFrame(
			SwapchainHandle swapChain,
			Span<ICommandBuffer*> graphics_buffers,
			Span<FenceHandle> wait_fences = {}) override;

		void WaitForIdle() override;

		ICommandBuffer* BeginCommandBuffer(CommandQueueType queue_type) override;
		FenceHandle Submit(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences) override;

		VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count);
		~VulkanSubmissionManager() override = default;

	};
}

