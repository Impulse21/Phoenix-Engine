#pragma once

#include <volk.h>

#include <PhxRhi/PhxRhi_Types.h>
#include <PhxCore/Result.h>

namespace phx::rhi::vulkan
{
    constexpr size_t UPLOAD_RING_BUFFER_SIZE = 64 * 1024 * 1024;
    constexpr size_t MAX_INFLIGHT_FRAMES = 2;

    struct StagingRingBuffer
    {
        BufferHandle buffer_handle = {};
        std::byte* mapped_ptr = nullptr;
        uint64_t size = 0ull;
        uint64_t mask = 0ull;
        uint64_t head = 0ull;
        std::atomic_uint64_t tail;

        Result<StagingBlock> Allocate(uint64_t alloc_size, uint32_t alignment);
        void Initialize();
        void Shutdown();
    };

    struct CommandPool
    {
        CommandQueueType queue_type;
        VkCommandPool vk_cmd_pool;
        std::vector<VkCommandBuffer> cmd_buffer_pool;
        std::vector<VkCommandBuffer> free_cmd_buffers;

        VkCommandBuffer GetFreeBuffer(uint32_t thread_id);
    };

    struct PerThreadData
    {
        uint32_t thread_id = 0;
        EnumArray<CommandPool, CommandQueueType> command_pools;
        std::vector<BufferHandle> active_one_off_buffers;
        StagingRingBuffer staging_ring_buffer;

        std::vector<VkCommandBuffer> active_command_buffers;

        void Initialize(uint32_t thread_id);
        void Shutdown();

        StagingBlock RequestStagingBlock(size_t size, uint32_t alignment);
        StagingBlock CreateOneShotUploadBuffer(size_t size, uint32_t alignment);
    };

    struct SubmissionContext
    {
        struct PerQueueSync
        {
            VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
            std::atomic_uint64_t fence_counter = 1;
        };
        EnumArray<PerQueueSync, CommandQueueType> per_queue_syncs;

        StaticArray<FenceHandle, MAX_INFLIGHT_FRAMES> frame_fences = { .data = {{}, {}} };

        size_t frame_number = 0;
        size_t num_threads = 0;
        std::unique_ptr<PerThreadData[]> per_thread_data;

        struct InflightCommandBuffer
        {
            VulkanCommandBuffer* buffer;
            FenceHandle fence_handle;
        };
        std::vector<InflightCommandBuffer> inflight_cmd_queue;
        std::mutex inflight_commands_queue_mutex;

        struct InflightUpload
        {
            uint64_t fence_value = 0;
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

        bool Initialize(size_t thread_count);
        void Shutdown();

        size_t GetCurrentFrameIndex() const { return frame_number % MAX_INFLIGHT_FRAMES; }

        void ReclaimFinishedCommandBuffers();
        void ReclaimFinishedUploads();
        void RetireCommandBuffers(Span<VulkanCommandBuffer*> command_buffers, FenceHandle fence_value);

        FenceHandle SubmitInternal(
            CommandQueueType queue_type,
            Span<VkCommandBuffer> cmd_buffers,
            Span<FenceHandle> wait_fences,
            Span<VkSemaphore> binary_wait_sems,
            Span<VkSemaphore> binary_signal_sems,
            VkPipelineStageFlags flags);
    };
}