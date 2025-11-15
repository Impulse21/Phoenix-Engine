#pragma once

#include <PhxCore/StaticArray.h>
#include <PhxCore/Result.h>

#include <PhxRhi/ISubmissionManager.h>

#include "VulkanCommandBuffer.h"
#include <volk.h>

#include <deque>

namespace phx::rhi
{
	constexpr size_t UPLOAD_RING_BUFFER_SIZE = 64_MiB;
	static_assert((UPLOAD_RING_BUFFER_SIZE& (UPLOAD_RING_BUFFER_SIZE - 1)) == 0, "Buffer Size must be a power of 2");

	struct VulkanBackend;
	struct VulkanResourceManager;
	struct VulkanSubmissionManager;

	struct StagingRingBuffer
	{
		BufferHandle buffer_handle = {};
		std::byte* mapped_ptr = nullptr;
		uint64_t size = 0ull;
		uint64_t mask = 0ull;
		uint64_t head = 0ull;

		// Main thread writes tot his
		std::atomic_uint64_t tail;

		Result<StagingBlock> Allocate(uint64_t alloc_size, uint32_t alignment);
		void Initialize(VulkanSubmissionManager* sub_manager);
		void Shutdown(VulkanSubmissionManager* sub_manager);
	};

	struct PerThreadData
	{
		struct CommandPool
		{
			CommandQueueType queue_type;
			VulkanResourceManager* vulkan_resource_manager;
			VulkanBackend* vulkan_backend;
			VkCommandPool vk_cmd_pool;
			std::vector<std::unique_ptr<phx::rhi::VulkanCommandBuffer>> buffer_pool;
			std::vector<phx::rhi::VulkanCommandBuffer*> free_buffers;

			phx::rhi::VulkanCommandBuffer* GetFreeBuffer(uint32_t thread_id);
		};

		uint32_t thread_id = 0;
		VulkanSubmissionManager* sub_manager;

		EnumArray<CommandPool, CommandQueueType> command_pools;

		// -- Upload manager info ---
		// TODO: Move to it's own class.

		std::vector<BufferHandle> active_one_off_buffers;
		StagingRingBuffer staging_ring_buffer;

		StagingBlock RequestStagingBlock(size_t size, uint32_t alignment);
		StagingBlock CreateOneShotUploadBuffer(size_t size, uint32_t alignment);

		void Initialize(VulkanSubmissionManager* sub_manager, uint32_t thread_id);
		void Shutdown();
	};

	struct VulkanSubmissionManager : public ISubmissionManager
	{

		VulkanBackend* vulkan_backend;
		VulkanResourceManager* vulkan_resource_manager;
		std::unique_ptr<PerThreadData[]> per_thread_data;

		struct PerQueueSync
		{
			VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
			std::atomic_uint64_t fence_counter = 1;
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
		std::vector<InflightCommandBuffer> inflight_cmd_queue;
		std::mutex inflight_commands_queue_mutex;

		struct InflightUpload
		{
			uint64_t fence_value = 0;;
			uint32_t thread_id;
			uint64_t head_offset;
		};
		std::vector<InflightUpload> inflight_upload_queue;

		struct PendingDeletion
		{
			uint64_t fence_value = 0;
			BufferHandle buffer = {};
		};
		std::vector<PendingDeletion> pending_one_off_deletions;
		std::mutex upload_tracking_mutex;

		// -- Interface implementation ---
		bool Initialize() override;
		void Shutdown() override;

		void BeginFrame(SwapchainHandle swapChain) override;
		void EndFrame(
			SwapchainHandle swapChain,
			Span<ICommandBuffer*> graphics_buffers,
			Span<FenceHandle> wait_fences = {}) override;

		void WaitForIdle() override;
		bool IsFenceCompleted(FenceHandle handle) override;

		StagingBlock RequestStagingMemory(uint32_t size, uint32_t aligmnet = 16) override;

		ICommandBuffer* BeginCommandBuffer(CommandQueueType queue_type) override;
		FenceHandle Submit(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences = {}) override;

		VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count);
		~VulkanSubmissionManager() override = default;

	private:
		friend PerThreadData;

		size_t GetCurrentFrameIndex() { return frame_number % kBufferCount; }
		void RetireCommandBuffers(Span<ICommandBuffer*> command_buffers, FenceHandle fence_value);
		void ReclaimFinishedCommandBuffers();
		void ReclaimFinishedUploads();

		FenceHandle SubmitInternal(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences,
			Span<VkSemaphore> binary_wait_sems,
			Span<VkSemaphore> binary_signal_sems,
			VkPipelineStageFlags flags);
	};
}

