#include "PhxRhi/PhxRhi_pch.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxRhi/PhxRhi_Thread.h>

#include "VulkanInternal.h"

#include "VulkanSubmissionCtx.h"

using namespace phx;
using namespace phx::rhi;
DynamicAllocation phx::rhi::AllocDynamic(uint32_t size, uint32_t alignment)
{
    const uint32_t thread_id = g_rhi_thread_index;
    vulkan::SubmissionContext& submission_ctx = g_vulkan.submission;

    vulkan::PerThreadData& thread_data = submission_ctx.per_thread_data[thread_id];
    
    vulkan::TempAllocation alloc = thread_data.gpu_linear_allocator.Allocate(g_vulkan.dynamic_upload_ring, size, alignment);

    return DynamicAllocation{
        .ptr = alloc.mapped_data + alloc.byte_offset,
        .device_address = alloc.device_address + alloc.byte_offset
	};
}


void phx::rhi::BeginFrame(SwapchainHandle swapchain)
{
    vulkan::SubmissionContext& submission_ctx = g_vulkan.submission;
    const size_t frame_index = submission_ctx.GetCurrentFrameIndex();
    const FenceHandle frame_to_wait_for = submission_ctx.frame_fences[frame_index];

    if (frame_to_wait_for.value > 0)
    {
        VkSemaphore wait_timline_sem = submission_ctx.per_queue_syncs[frame_to_wait_for.queue_type].vk_timeline_semaphore;
        VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &wait_timline_sem,
            .pValues = &frame_to_wait_for.value
        };

        vkWaitSemaphores(
            g_vulkan.vk_device,
            &wait_info,
            UINT64_MAX
        );
    }

    submission_ctx.ReclaimFinishedCommandBuffers();
    submission_ctx.ReclaimFinishedUploads();
    g_vulkan.dynamic_upload_ring.BeginFrame(frame_to_wait_for.value);

    for (size_t i = 0; i < submission_ctx.num_threads; ++i)
        submission_ctx.per_thread_data[i].gpu_linear_allocator.Reset();

    VulkanSwapchain* swapchain_impl = g_vulkan.swapchain_pool.GetCold(swapchain);
    if (!swapchain_impl)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    VulkanSwapchainFrame* impl_frame = g_vulkan.swapchain_pool.GetHot(swapchain);
    impl_frame->vk_image_available_sem = swapchain_impl->vk_image_available_sem[frame_index];

    uint32_t image_index;
    vkAcquireNextImageKHR(
        g_vulkan.vk_device,
        swapchain_impl->vk_swapchain,
        UINT64_MAX,
        impl_frame->vk_image_available_sem,
        VK_NULL_HANDLE,
        &image_index);

    impl_frame->image_index = static_cast<uint8_t>(image_index);
    impl_frame->vk_render_finished_sem = swapchain_impl->vk_render_finished_sem[image_index];
    impl_frame->vk_image = swapchain_impl->vk_images[image_index];
    impl_frame->vk_image_view = swapchain_impl->vk_image_views[image_index];
    impl_frame->resource_state = ResourceStates::Unknown;
}

void phx::rhi::EndFrame(
    SwapchainHandle swapchain,
    Span<CmdHandle> graphics_buffers,
    Span<FenceHandle> wait_fences)
{
    if (graphics_buffers.IsEmpty())
        return;

    VulkanSwapchainFrame* swapchain_impl_frame = g_vulkan.swapchain_pool.GetHot(swapchain);
    if (!swapchain_impl_frame)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    vulkan::SubmissionContext& submission_ctx = g_vulkan.submission;
    const uint64_t current_fame_index = submission_ctx.GetCurrentFrameIndex();
    submission_ctx.frame_fences[current_fame_index] =
        submission_ctx.SubmitInternal(
            CommandQueueType::Graphics,
            graphics_buffers,
            wait_fences,
            { swapchain_impl_frame->vk_image_available_sem },
            { swapchain_impl_frame->vk_render_finished_sem },
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);


    uint64_t fence_value = submission_ctx.frame_fences[current_fame_index].value;
    g_vulkan.dynamic_upload_ring.EndFrame(fence_value);

    const uint32_t image_index = static_cast<uint32_t>(swapchain_impl_frame->image_index);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain_impl_frame->vk_render_finished_sem,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_impl_frame->vk_swapchain,
        .pImageIndices = &image_index,
    };

    VulkanQueue& queue = g_vulkan.queues[CommandQueueType::Graphics];
    vkQueuePresentKHR(queue.vk_queue, &present_info);

    submission_ctx.frame_number++;
}

FenceHandle phx::rhi::Submit(
    CommandQueueType queue_type,
    Span<CmdHandle> cmd_buffers,
    Span<FenceHandle> wait_fences)
{
    // Using VK_PIPELINE_STAGE_ALL_COMMANDS_BIT as it's a safe choice.
    return g_vulkan.submission.SubmitInternal(queue_type, cmd_buffers, wait_fences, {}, {}, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

StagingBlock phx::rhi::RequestStagingMemory(uint32_t size, uint32_t aligmnet)
{
    vulkan::SubmissionContext& submission_ctx = g_vulkan.submission;
    const uint32_t thread_id = g_rhi_thread_index;
    vulkan::PerThreadData& thread_data = submission_ctx.per_thread_data[thread_id];

    return thread_data.RequestStagingBlock(size, aligmnet);
}

void phx::rhi::WaitForIdle()
{
    PHX_CORE_ASSERT(g_vulkan.vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(g_vulkan.vk_device);
}

bool phx::rhi::IsFenceCompleted(FenceHandle fence_handle)
{
    vulkan::SubmissionContext& submission_ctx = g_vulkan.submission;
    vulkan::PerQueueSync& queue_sync = submission_ctx.per_queue_syncs[fence_handle.queue_type];

    uint64_t completed_value = 0;

    VkResult result = vkGetSemaphoreCounterValue(
        g_vulkan.vk_device,
        queue_sync.vk_timeline_semaphore,
        &completed_value);

    PHX_CORE_ASSERT(result == VK_SUCCESS, "Failed to retrieve timeline semaphore's completed value")
        if (result != VK_SUCCESS)
        {
            return false;
        }

    return fence_handle.value <= completed_value;
}

bool vulkan::SubmissionContext::Initialize(size_t thread_count)
{
    PHX_PROFILE;
    if (g_vulkan.vk_features_1_2.timelineSemaphore != VK_TRUE)
    {
        PHX_RHI_ERROR("Required VK 1.2 feature - Timeline Semaphore is not available on this device.");
        return false;
    }
    VkSemaphoreTypeCreateInfo timeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = NULL,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_create_info,
        .flags = 0,
    };

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        VkResult result = vkCreateSemaphore(g_vulkan.vk_device, &semaphore_create_info, NULL, &per_queue_syncs[q].vk_timeline_semaphore);
        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create queue timeline semaphore");
    }

    num_threads = thread_count;
    PHX_RHI_INFO("Initializing Per thread Command data.");
    per_thread_data = std::make_unique<PerThreadData[]>(num_threads);
    for (size_t i = 0; i < num_threads; ++i)
    {
        per_thread_data[i].Initialize(i);
    }

    return true;
}
void vulkan::SubmissionContext::Shutdown()
{
    WaitForIdle();
    g_vulkan.deferred_delete_queue.Flush(~0u);

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        vkDestroySemaphore(g_vulkan.vk_device, per_queue_syncs[q].vk_timeline_semaphore, nullptr);
    }
    for (size_t i = 0; i < num_threads; ++i)
    {
        per_thread_data[i].Shutdown();
    }
}


Result<StagingBlock> vulkan::StagingRingBuffer::Allocate(uint64_t alloc_size, uint32_t alignment)
{
    uint64_t aligned_head = AlignUp(alloc_size, alignment);
    uint64_t new_head = aligned_head + alloc_size;
    uint64_t current_tail = tail.load(std::memory_order_acquire);
    if ((new_head - current_tail) > size)
        return make_unexpected(1ull);

    head = new_head;

    StagingBlock staging_block = {
        .data_ptr = mapped_ptr + (aligned_head & mask),
        .size = alloc_size,
        .buffer_handle = buffer_handle,
        .gpu_offset = (aligned_head & mask)
    };

    return staging_block;
}

void vulkan::StagingRingBuffer::Initialize()
{
    size = UPLOAD_RING_BUFFER_SIZE;
    mask = size - 1;
    head = 0;

    buffer_handle = rhi::CreateBuffer({
        .DebugName = "One_shot_bufffer",
        .Size = static_cast<uint32_t>(UPLOAD_RING_BUFFER_SIZE),
        .Usage = Usage::Upload,
        .MiscFlags = ResourceMiscFlags::BufferRaw,
        .InitialState = ResourceStates::CopySource
        });

    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(buffer_handle);
    mapped_ptr = static_cast<std::byte*>(vulkan_buffer->mapped_data);
}

void vulkan::StagingRingBuffer::Shutdown()
{
    rhi::DeleteBufferImmediate(buffer_handle);
}

uint32_t vulkan::CommandPool::GetFreeBufferIndex()
{
    if (!free_cmd_buffers.empty())
    {
        uint32_t free_index = free_cmd_buffers.back();
        free_cmd_buffers.pop_back();

        return free_index;
    }

    VkCommandBufferAllocateInfo cmd_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    // 2. Actually create the handle
    VkCommandBuffer vk_buffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(g_vulkan.vk_device, &cmd_alloc_info, &vk_buffer);

    uint32_t free_index = cmd_buffer_pool.size();
    cmd_buffer_pool.push_back(vk_buffer);

    return free_index;
}

StagingBlock vulkan::PerThreadData::RequestStagingBlock(size_t size, uint32_t alignment)
{
    if (size > UPLOAD_RING_BUFFER_SIZE)
        return CreateOneShotUploadBuffer(size, alignment);

    Result<StagingBlock> result = staging_ring_buffer.Allocate(size, alignment);
    if (result)
    {
        return result.GetValue();
    }

    g_vulkan.submission.ReclaimFinishedUploads();

    result = staging_ring_buffer.Allocate(size, alignment);
    if (result)
    {
        return result.GetValue();
    }

    PHX_RHI_WARN("Staging buffer fragmented! Promoting to one-off");
    return CreateOneShotUploadBuffer(size, alignment);
}

StagingBlock vulkan::PerThreadData::CreateOneShotUploadBuffer(size_t size, uint32_t alignment)
{
    const size_t aligned_size = AlignUp(size, alignment);
    BufferHandle buffer_handle = rhi::CreateBuffer({
        .DebugName = "One_shot_bufffer",
        .Size = static_cast<uint32_t>(aligned_size),
        .Usage = Usage::Upload,
        .MiscFlags = ResourceMiscFlags::BufferRaw,
        .InitialState = ResourceStates::CopySource
        });

    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(buffer_handle);
    StagingBlock block = {
        .data_ptr = vulkan_buffer->mapped_data,
        .size = size,
        .buffer_handle = buffer_handle,
        .gpu_offset = 0
    };

    return block;
}

void vulkan::PerThreadData::Initialize(uint32_t thread_id)
{
    this->thread_id = thread_id;

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        CommandPool& pool = command_pools[q];
        pool.queue_type = static_cast<CommandQueueType>(q);

        VkCommandPoolCreateInfo pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = g_vulkan.queues[q].vk_queue_family,
        };

        VkResult result = vkCreateCommandPool(g_vulkan.vk_device, &pool_info, nullptr, &pool.vk_cmd_pool);
        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create command pool");
    }

    staging_ring_buffer.Initialize();
}

void vulkan::PerThreadData::Shutdown()
{
    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        vkDestroyCommandPool(g_vulkan.vk_device, command_pools[q].vk_cmd_pool, nullptr);
    }

    staging_ring_buffer.Shutdown();
}

void vulkan::SubmissionContext::ReclaimFinishedCommandBuffers()
{
    EnumArray<uint64_t, CommandQueueType> completed_values = {};
    VkResult result;
    for (size_t i = 0; i < static_cast<uint32_t>(CommandQueueType::Count); ++i)
    {
        // This is the key function:
        result = vkGetSemaphoreCounterValue(
            g_vulkan.vk_device,
            per_queue_syncs[i].vk_timeline_semaphore,
            &completed_values[i]);

        if (result != VK_SUCCESS)
        {
            PHX_RHI_ERROR("Failed to get semaphore fence value");
            completed_values[i] = 0; // Or last known good value
        }
    }

    std::scoped_lock _(inflight_commands_queue_mutex);
    std::erase_if(inflight_cmd_queue,
        [&](const InflightCommandBuffer& pending) {
            const FenceHandle& fence = pending.fence_handle;

            if (fence.value <= completed_values[fence.queue_type])
            {
                CommandQueueType queue_type;
                uint32_t index;
                uint32_t thread_id;
                DecodeCmdHande(pending.buffer_handle, queue_type, thread_id, index);

                PerThreadData& thread_data = per_thread_data[thread_id];
                vulkan::CommandPool& pool = thread_data.command_pools[queue_type];
                pool.free_cmd_buffers.push_back(index);

                return true;
            }

            return false;
        });
}

void vulkan::SubmissionContext::ReclaimFinishedUploads()
{
    uint64_t completed_fence_value = 0;

    VkResult result = vkGetSemaphoreCounterValue(
        g_vulkan.vk_device,
        per_queue_syncs[CommandQueueType::Copy].vk_timeline_semaphore,
        &completed_fence_value);

    if (result != VK_SUCCESS)
    {
        PHX_RHI_ERROR("Failed to get semaphore fence value");
        completed_fence_value = 0; // Or last known good value
    }

    static thread_local std::vector<uint64_t> s_new_tail_values(num_threads, 0);

    std::scoped_lock _(upload_tracking_mutex);

    auto it = inflight_upload_queue.begin();
    while (it != inflight_upload_queue.end())
    {
        InflightUpload& upload = *it;

        // Check if this upload's fence is complete
        if (upload.fence_value <= completed_fence_value)
        {
            uint64_t& current_max = s_new_tail_values[upload.thread_id];
            current_max = std::max(current_max, upload.head_offset);

            it = inflight_upload_queue.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (uint32_t thread_id = 0; thread_id < num_threads; ++thread_id)
    {
        if (s_new_tail_values[thread_id] > 0)
        {
            per_thread_data[thread_id].staging_ring_buffer.tail.store(
                s_new_tail_values[thread_id],
                std::memory_order_release);

            // Reset our temp value
            s_new_tail_values[thread_id] = 0;
        }
    }

    std::erase_if(pending_one_off_deletions,
        [&](const PendingDeletion& pending) {

            if (pending.fence_value <= completed_fence_value)
            {
                DeleteBufferImmediate(pending.buffer);
                return true;
            }

            return false;
        });
}

void vulkan::SubmissionContext::RetireCommandBuffers(Span<CmdHandle> command_buffers, FenceHandle fence_value)
{
    std::scoped_lock _(inflight_commands_queue_mutex);

    for (auto cmd_buffer : command_buffers)
    {
        inflight_cmd_queue.push_back({
                .buffer_handle = cmd_buffer,
                .fence_handle = fence_value,
            });
    }
}

FenceHandle vulkan::SubmissionContext::SubmitInternal(
    CommandQueueType queue_type,
    Span<CmdHandle> cmd_buffer_handles,
    Span<FenceHandle> wait_fences,
    Span<VkSemaphore> binary_wait_sems,
    Span<VkSemaphore> binary_signal_sems,
    VkPipelineStageFlags flags)
{

    PerQueueSync& queue_sync = per_queue_syncs[queue_type];
    const FenceHandle fence_handle = {
        .value = queue_sync.fence_counter.fetch_add(1),
        .queue_type = queue_type
    };

    static thread_local std::vector<VkCommandBuffer> s_vk_cmd_buffers;
    {
        s_vk_cmd_buffers.clear();
        s_vk_cmd_buffers.reserve(cmd_buffer_handles.size());

        for (auto& cmd_buffer_handle : cmd_buffer_handles)
        {
            VkCommandBuffer vk_handle = ResolveCmdBuffer(cmd_buffer_handle);
            
            vkEndCommandBuffer(vk_handle);
            s_vk_cmd_buffers.push_back(vk_handle);
        }
    }

    static thread_local std::vector<uint64_t> s_wait_fence_values;
    {
        s_wait_fence_values.clear();
        s_wait_fence_values.reserve(binary_wait_sems.size() + wait_fences.size());

        for (size_t i = 0; i < binary_wait_sems.size(); ++i)
            s_wait_fence_values.push_back(0);

        for (auto& wait_fence : wait_fences)
            s_wait_fence_values.push_back(wait_fence.value);
    }

    static thread_local std::vector<VkSemaphore> s_wait_semaphores;
    {
        s_wait_semaphores.clear();
        s_wait_semaphores.reserve(binary_wait_sems.size() + wait_fences.size());

        for (auto& binary_semaphore : binary_wait_sems)
            s_wait_semaphores.push_back(binary_semaphore);

        for (size_t i = 0; i < wait_fences.size(); ++i)
            s_wait_semaphores.push_back(queue_sync.vk_timeline_semaphore);
    }

    static thread_local std::vector<VkPipelineStageFlags> s_wait_stages;
    {
        s_wait_stages.clear();
        s_wait_stages.resize(binary_wait_sems.size() + wait_fences.size());

        std::fill(s_wait_stages.begin(), s_wait_stages.end(), flags);
    }

    static thread_local std::vector<uint64_t> s_signal_fence_values;
    {
        s_signal_fence_values.clear();
        s_signal_fence_values.reserve(binary_signal_sems.size() + 1);

        for (size_t i = 0; i < binary_signal_sems.size(); ++i)
            s_signal_fence_values.push_back(0);

        s_signal_fence_values.push_back(fence_handle.value);
    }

    static thread_local std::vector<VkSemaphore> s_signal_semaphores;
    {
        s_signal_semaphores.clear();
        s_signal_semaphores.reserve(binary_signal_sems.size() + 1);

        for (auto& binary_signal_sem : binary_signal_sems)
            s_signal_semaphores.push_back(binary_signal_sem);

        s_signal_semaphores.push_back(queue_sync.vk_timeline_semaphore);
    }

    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = static_cast<uint32_t>(s_wait_fence_values.size()),
        .pWaitSemaphoreValues = s_wait_fence_values.data(),
        .signalSemaphoreValueCount = static_cast<uint32_t>(s_signal_fence_values.size()),
        .pSignalSemaphoreValues = s_signal_fence_values.data(),
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,

        .waitSemaphoreCount = static_cast<uint32_t>(s_wait_semaphores.size()),
        .pWaitSemaphores = s_wait_semaphores.data(),
        .pWaitDstStageMask = s_wait_stages.data(),

        .commandBufferCount = static_cast<uint32_t>(s_vk_cmd_buffers.size()),
        .pCommandBuffers = s_vk_cmd_buffers.data(),

        .signalSemaphoreCount = static_cast<uint32_t>(s_signal_semaphores.size()),
        .pSignalSemaphores = s_signal_semaphores.data(),
    };

    // Get queue
    VulkanQueue& queue = g_vulkan.queues[queue_type];
    VkResult result = vkQueueSubmit(queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE);
    //PHX_CORE_ASSERT(result != VK_ERROR_DEVICE_LOST, "GPU HAS CRASHED");
    (void)result;
    RetireCommandBuffers(cmd_buffer_handles, fence_handle);

    if (queue_type == CommandQueueType::Copy)
    {
        uint32_t thread_index = g_rhi_thread_index;
        PerThreadData& thread_data = per_thread_data[thread_index];
        std::scoped_lock _(upload_tracking_mutex);

        InflightUpload& inflight_data = inflight_upload_queue.emplace_back();
        inflight_data.fence_value = fence_handle.value;
        inflight_data.thread_id = thread_index;
        inflight_data.head_offset = thread_data.staging_ring_buffer.head;

        for (auto& one_off_buffer : thread_data.active_one_off_buffers)
        {
            PendingDeletion& pending = pending_one_off_deletions.emplace_back();
            pending.fence_value = fence_handle.value;
            pending.buffer = one_off_buffer;
        }
        thread_data.active_one_off_buffers.clear();
    }

    return fence_handle;
}