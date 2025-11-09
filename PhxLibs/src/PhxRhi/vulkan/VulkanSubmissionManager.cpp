#include "PhxRhi/PhxRhi_pch.h"
#include <PhxRhi/PhxRhi_Thread.h>
#include "VulkanSubmissionManager.h"

#include "VulkanBackend.h"
#include "VulkanResourceManager.h"

#include "volk.h"

using namespace phx;
using namespace phx::rhi;

VulkanSubmissionManager::VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count)
    : vulkan_backend(vulkan_backend)
    , vulkan_resource_manager(vulkan_resource_manager)
    , num_threads(thread_count)
    , per_thread_cmd_pool(std::make_unique<PerThreadData[]>(thread_count))
{

}

bool VulkanSubmissionManager::Initialize()
{
    PHX_PROFILE;
    if (vulkan_backend->vk_features_1_2.timelineSemaphore != VK_TRUE)
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

    VkSemaphoreCreateInfo image_available_semaphore_info = {};
    image_available_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (int i = 0; i < kBufferCount; ++i)
    {
        // Create the semaphore and store it in the array
        VkResult result = vkCreateSemaphore(vulkan_backend->vk_device, &image_available_semaphore_info, nullptr, &image_available_semaphores[i]);

        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create queue timeline semaphore");
    }

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_create_info,
        .flags = 0,
    };
    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        VkResult result = vkCreateSemaphore(vulkan_backend->vk_device, &semaphore_create_info, NULL, &per_queue_syncs[q].vk_timeline_semaphore);
        if (result != VK_SUCCESS)
            PHX_RHI_ERROR("Failed to create queue timeline semaphore");
    }

    PHX_RHI_INFO("Initializing Per thread Command data.");
    for (size_t i = 0; i < num_threads; ++i)
    {
        PerThreadData& thread_data = per_thread_cmd_pool[i];

        for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
        {

            VkCommandPoolCreateInfo pool_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = vulkan_backend->queues[q].vk_queue_family,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
            };

            VkResult result = vkCreateCommandPool(vulkan_backend->vk_device, &pool_info, nullptr, &thread_data.command_pools[q].vk_cmd_pool);
            if (result != VK_SUCCESS)
                PHX_RHI_ERROR("Failed to create command pool");

        }
    }

    return true;
}

void phx::rhi::VulkanSubmissionManager::Shutdown()
{
    WaitForIdle();
    vulkan_resource_manager->RunGarbageCollection(~0u);

    for (int i = 0; i < kBufferCount; ++i)
    {
        vkDestroySemaphore(vulkan_backend->vk_device, image_available_semaphores[i], nullptr);
    }

    for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
    {
        vkDestroySemaphore(vulkan_backend->vk_device, per_queue_syncs[q].vk_timeline_semaphore, nullptr);
    }

    for (size_t i = 0; i < num_threads; ++i)
    {
        PerThreadData& thread_data = per_thread_cmd_pool[i];

        for (size_t q = 0; q < static_cast<size_t>(CommandQueueType::Count); ++q)
        {
            vkDestroyCommandPool(vulkan_backend->vk_device, thread_data.command_pools[q].vk_cmd_pool, nullptr);
        }
    }

    per_thread_cmd_pool.reset();
}

void phx::rhi::VulkanSubmissionManager::BeginFrame(SwapchainHandle swapchain)
{
    FenceHandle frame_to_wait_for = frame_fences[GetCurrentFrameIndex()];
    if (frame_to_wait_for.value > 0)
    {
        VkSemaphore wait_timline_sem = per_queue_syncs[frame_to_wait_for.queue_type].vk_timeline_semaphore;
        VkSemaphoreWaitInfo wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &wait_timline_sem,
            .pValues = &frame_to_wait_for.value
        };

        VkResult result = vkWaitSemaphores(
            vulkan_backend->vk_device,
            &wait_info,
            UINT64_MAX
        );
    }

    ReclaimFinishedCommandBuffers();

    VulkanSwapchain* swapchain_impl = vulkan_resource_manager->swapchain_pool.GetHot(swapchain);
    if (!swapchain_impl)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    uint32_t image_index;
    vkAcquireNextImageKHR(
        vulkan_backend->vk_device,
        swapchain_impl->vk_swapchain,
        UINT64_MAX,
        image_available_semaphores[GetCurrentFrameIndex()],
        VK_NULL_HANDLE,
        &image_index
    );

    swapchain_impl->buffer_index = static_cast<uint8_t>(image_index);
}

void phx::rhi::VulkanSubmissionManager::EndFrame(
    SwapchainHandle swapchain,
    Span<ICommandBuffer*> graphics_buffers,
    Span<FenceHandle> wait_fences)
{
    const FenceHandle fence_handle = 
        SubmitInternal(
            CommandQueueType::Graphics,
            graphics_buffers,
            wait_fences,
            { image_available_semaphores[GetCurrentFrameIndex()] },
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    VulkanSwapchain* swapchain_impl = vulkan_resource_manager->swapchain_pool.GetHot(swapchain);
    if (!swapchain_impl)
    {
        PHX_RHI_CRITICAL("Unable to local swapchain when ending frame.");
        return;
    }

    PerQueueSync& queue_sync = per_queue_syncs[CommandQueueType::Graphics];
    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = 1,
        .pWaitSemaphoreValues = &fence_handle.value,
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = 0, // Present doesn't signal
    };

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &queue_sync.vk_timeline_semaphore,
        .pNext = &timeline_info,
        .swapchainCount = 1,
        .pSwapchains = &swapchain_impl->vk_swapchain,
        .pImageIndices = &swapchain_impl->vk_swapchain_image_index,
    };

    VulkanBackend::Queue& queue = vulkan_backend->queues[CommandQueueType::Graphics];
    vkQueuePresentKHR(queue.vk_queue, &present_info);
    frame_fences[GetCurrentFrameIndex()] = fence_handle;
    frame_number++;
}

void VulkanSubmissionManager::WaitForIdle()
{
    PHX_CORE_ASSERT(vulkan_backend->vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(vulkan_backend->vk_device);
}

ICommandBuffer* VulkanSubmissionManager::BeginCommandBuffer(CommandQueueType queue_type)
{
    const uint32_t thread_index = g_rhi_thread_index;

    PerThreadData& thread_data = per_thread_cmd_pool[thread_index];
    PerThreadData::CommandPool& pool = thread_data.command_pools[queue_type];

    return pool.GetFreeBuffer();
}

FenceHandle VulkanSubmissionManager::Submit(
    CommandQueueType queue_type,
    Span<ICommandBuffer*> cmd_buffers,
    Span<FenceHandle> wait_fences)
{
    // Using VK_PIPELINE_STAGE_ALL_COMMANDS_BIT as it's a safe choice.
    return SubmitInternal(queue_type, cmd_buffers, wait_fences, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

phx::rhi::VulkanCommandBuffer* phx::rhi::VulkanSubmissionManager::PerThreadData::CommandPool::GetFreeBuffer()
{
    if (!free_buffers.empty())
    {
        phx::rhi::VulkanCommandBuffer* buffer = free_buffers.back();
        free_buffers.pop_back();

        return buffer;
    }

    auto& vulkan_cmd_buffer = buffer_pool.emplace_back(std::make_unique<VulkanCommandBuffer>());
    return vulkan_cmd_buffer.get();
}

void phx::rhi::VulkanSubmissionManager::RetireCommandBuffers(Span<ICommandBuffer*> command_buffers, FenceHandle fence_value)
{
    std::scoped_lock _(inglight_commands_queue_mutex);

    for (auto cmd_buffer : command_buffers)
    {
        inflight_cmd_queue.push_back({
                .buffer = static_cast<VulkanCommandBuffer*>(command_buffers),
                .fence_handle = fence_value,
            });
    }
}

void phx::rhi::VulkanSubmissionManager::ReclaimFinishedCommandBuffers()
{
    // TODO: I am here.
}

FenceHandle phx::rhi::VulkanSubmissionManager::SubmitInternal(
    CommandQueueType queue_type,
    Span<ICommandBuffer*> cmd_buffers,
    Span<FenceHandle> wait_fences,
    Span<VkSemaphore> binary_semaphores,
    VkPipelineStageFlags flags)
{
    PerQueueSync& queue_sync = per_queue_syncs[queue_type];
    const FenceHandle fence_handle = {
        .value = queue_sync.fence_counter.fetch_add(1),
        .queue_type = queue_type
    };

    static thread_local std::vector<VkCommandBuffer> s_vk_cmd_buffers;
    s_vk_cmd_buffers.clear();
    s_vk_cmd_buffers.reserve(cmd_buffers.size());

    for (auto& cmd_buffer : cmd_buffers)
    {
        VkCommandBuffer vk_handle = static_cast<VulkanCommandBuffer*>(cmd_buffer)->vk_handle;
        s_vk_cmd_buffers.push_back(vk_handle);
    }

    static thread_local std::vector<uint64_t> s_wait_fence_values;
    s_wait_fence_values.clear();
    s_wait_fence_values.reserve(binary_semaphores.size() + wait_fences.size());

    for (size_t i = 0; i < binary_semaphores.size(); ++i)
        s_wait_fence_values.push_back(0);

    for (auto& wait_fence : wait_fences)
        s_wait_fence_values.push_back(wait_fence.value);

    static thread_local std::vector<VkSemaphore> s_wait_semaphores;
    s_wait_semaphores.clear();
    s_wait_semaphores.reserve(binary_semaphores.size() + wait_fences.size());

    for (auto& binary_semaphore : binary_semaphores)
        s_wait_semaphores.push_back(binary_semaphore);

    for (size_t i = 0; i < binary_semaphores.size(); ++i)
        s_wait_semaphores.push_back(queue_sync.vk_timeline_semaphore);


    static thread_local std::vector<VkPipelineStageFlags> s_wait_stages;
    s_wait_stages.clear();
    s_wait_stages.resize(binary_semaphores.size() + wait_fences.size());

    std::fill(s_wait_stages.begin(), s_wait_stages.end(), flags);

    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = static_cast<uint32_t>(wait_fences.size()),
        .pWaitSemaphoreValues = s_wait_fence_values.data(),
        .signalSemaphoreValueCount = 1,
        .pSignalSemaphoreValues = &fence_handle.value,
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,
        .commandBufferCount = static_cast<uint32_t>(s_vk_cmd_buffers.size()),
        .pCommandBuffers = s_vk_cmd_buffers.data(),
        .waitSemaphoreCount = static_cast<uint32_t>(wait_fences.size()),
        .pWaitSemaphores = s_wait_semaphores.data(),
        .pWaitDstStageMask = s_wait_stages.data(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &queue_sync.vk_timeline_semaphore, // Signal our one timeline
    };

    // Get queue
    VulkanBackend::Queue& queue = vulkan_backend->queues[queue_type];
    vkQueueSubmit(queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE);

    RetireCommandBuffers(cmd_buffers, fence_handle);

    return fence_handle;
}