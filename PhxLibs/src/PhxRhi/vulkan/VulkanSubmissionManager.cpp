#include "PhxRhi/PhxRhi_pch.h"
#include <PhxRhi/PhxRhi_Thread.h>
#include "VulkanSubmissionManager.h"

#include "VulkanBackend.h"
#include "VulkanResourceManager.h"

#include "volk.h"

using namespace phx;
using namespace phx::rhi;


namespace
{
    VulkanSubmissionManager::PerThreadData::CommandPool& GetPoolForType(
        VulkanSubmissionManager::PerThreadData& thread_data,
        CommandQueueType type)
    {
        switch (type)
        {
        case CommandQueueType::Graphics:
            return thread_data.graphics_cmd_pool;
        case CommandQueueType::Compute:
            return thread_data.compute_cmd_pool;
        case CommandQueueType::Copy:
            return thread_data.upload_cmd_pool;
        default:
            PHX_ASSERT(false, "Unexpected types");
            throw std::exception();
        }
    }
}

VulkanSubmissionManager::VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count)
    : vulkan_backend(vulkan_backend)
    , vulkan_resource_manager(vulkan_resource_manager)
    , num_threads(thread_count)
    , per_thread_cmd_pool(std::make_unique<PerThreadData[]>(thread_count))
{

}

bool VulkanSubmissionManager::Initialize()
{
    {
        PHX_PROFILE_SECTION("Vulkan::CreateFrameSyncObjects");
        VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; // Renamed to snake_case
        VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; // Renamed to snake_case
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < kBufferCount; ++i)
        {
            VkResult result = vkCreateSemaphore(vulkan_backend->vk_device, &semaphore_info, nullptr, &frames[i].present_semaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateSemaphore(vulkan_backend->vk_device, &semaphore_info, nullptr, &frames[i].render_semaphore);
            PHX_CORE_ASSERT(result == VK_SUCCESS);

            result = vkCreateFence(vulkan_backend->vk_device, &fence_info, nullptr, &frames[i].frame_fence);
            PHX_CORE_ASSERT(result == VK_SUCCESS);
        }

        PHX_RHI_INFO("Frame synchronization primitives created.");
    }

    PHX_RHI_INFO("Initializing Per thread Command data.");
    for (size_t i = 0; i < num_threads; ++i)
    {
        PerThreadData& thread_data = per_thread_cmd_pool[i];

        auto init_cmd_pool = [&](PerThreadData::CommandPool& pool, uint32_t queue_family) {
            VkCommandPoolCreateInfo pool_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = queue_family,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
            };

            VkResult result = vkCreateCommandPool(vulkan_backend->vk_device, &pool_info, nullptr, &pool.vk_cmd_pool);
            if (result != VK_SUCCESS)
                PHX_RHI_ERROR("Failed to create command pool");
        };

        init_cmd_pool(thread_data.graphics_cmd_pool, vulkan_backend->queue_gfx.vk_queue_family);
        init_cmd_pool(thread_data.compute_cmd_pool, vulkan_backend->queue_compute.vk_queue_family);
        init_cmd_pool(thread_data.upload_cmd_pool, vulkan_backend->queue_transfer.vk_queue_family);
    }

    return true;
}

void phx::rhi::VulkanSubmissionManager::Shutdown()
{
    WaitForIdle();
    vulkan_resource_manager->RunGarbageCollection(~0u);

    for (size_t i = 0; i < num_threads; ++i)
    {
        PerThreadData& thread_data = per_thread_cmd_pool[i];

        vkDestroyCommandPool(vulkan_backend->vk_device, thread_data.graphics_cmd_pool.vk_cmd_pool, nullptr);
        vkDestroyCommandPool(vulkan_backend->vk_device, thread_data.compute_cmd_pool.vk_cmd_pool, nullptr);
        vkDestroyCommandPool(vulkan_backend->vk_device, thread_data.upload_cmd_pool.vk_cmd_pool, nullptr);
    }
    per_thread_cmd_pool.reset();

    for (size_t i = 0; i < cMaxInflightFrames; ++i)
    {
        if (frames[i].frame_fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(vulkan_backend->vk_device, frames[i].frame_fence, nullptr);
            frames[i].frame_fence = VK_NULL_HANDLE;
        }

        if (frames[i].render_semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(vulkan_backend->vk_device, frames[i].render_semaphore, nullptr);
            frames[i].render_semaphore = VK_NULL_HANDLE;
        }

        if (frames[i].present_semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(vulkan_backend->vk_device, frames[i].present_semaphore, nullptr);
            frames[i].present_semaphore = VK_NULL_HANDLE;
        }
        PHX_RHI_INFO("Frame synchronization primitives destroyed.");
    }
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
    PerThreadData::CommandPool& pool = GetPoolForType(thread_data, queue_type);

    return pool.GetFreeBuffer();
}

FenceHandle VulkanSubmissionManager::Submit(
    CommandQueueType queue_type,
    Span<ICommandBuffer*> cmd_buffers,
    Span<FenceHandle> wait_fences)
{
    return FenceHandle();
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
