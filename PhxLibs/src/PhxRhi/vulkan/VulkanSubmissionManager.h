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

			EnumArray<CommandPool, CommandQueueType> command_pools;

		};
		std::unique_ptr<PerThreadData[]> per_thread_cmd_pool;

		struct PerQueueSync
		{
			VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
			std::atomic_uint64_t fence_counter = 0;
		};
		EnumArray<PerQueueSync, CommandQueueType> per_queue_syncs;

		StaticArray<VkSemaphore, kBufferCount> image_available_semaphores;
		StaticArray<FenceHandle, kBufferCount> frame_fences = { .data = {{}, {}} };

		size_t frame_number = 0;
		size_t num_threads = 0;

		struct InflightCommandBuffer
		{
			VulkanCommandBuffer*	buffer;
			FenceHandle				fence_handle;
		};
		std::deque<InflightCommandBuffer> inflight_cmd_queue;
		std::mutex inglight_commands_queue_mutex;

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

	private:
		size_t GetCurrentFrameIndex() { return frame_number % kBufferCount; }
		void RetireCommandBuffers(Span<ICommandBuffer*> command_buffers, FenceHandle fence_value);
		void ReclaimFinishedCommandBuffers();

		FenceHandle SubmitInternal(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences,
			Span<VkSemaphore> binary_semaphores,
			VkPipelineStageFlags flags);
	};
}

