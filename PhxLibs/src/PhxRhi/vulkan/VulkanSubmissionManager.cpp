#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanSubmissionManager.h"

#include "VulkanBackend.h"
#include "VulkanResourceManager.h"

#include "volk.h"

using namespace phx;
using namespace phx::rhi;


VulkanSubmissionManager::VulkanSubmissionManager(VulkanBackend* vulkan_backend, VulkanResourceManager* vulkan_resource_manager, size_t thread_count)
    : vulkan_backend(vulkan_backend)
    , vulkan_resource_manager(vulkan_resource_manager)
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
    return true;
}

void phx::rhi::VulkanSubmissionManager::Shutdown()
{
    WaitForIdle();
    vulkan_resource_manager->RunGarbageCollection(~0u);

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
    }

    PHX_RHI_INFO("Frame synchronization primitives destroyed.");
}

void CreateCommandPools()
{
    PHX_PROFILE_SECTION("Vulkan::CreateCommandPools");

    VkCommandPoolCreateInfo pool_info = {}; // Renamed to snake_case
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = vulkan_backend->queue_gfx.vk_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult result = vkCreateCommandPool(vulkan_backend->vk_device, &pool_info, nullptr, &VkContext::vk_graphics_command_pool);

    PHX_CORE_ASSERT(result == VK_SUCCESS);
    PHX_CORE_INFO("[RHI] Graphics Command Pool created.");
    // Create other command pools (compute, transfer) if needed
}

void DestroyCommandPools()
{
    if (VkContext::vk_graphics_command_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(vulkan_backend->vk_device, VkContext::vk_graphics_command_pool, nullptr);
        VkContext::vk_graphics_command_pool = VK_NULL_HANDLE;
        PHX_CORE_INFO("[RHI] Graphics Command Pool destroyed.");
    }
}


void phx::rhi::VulkanSubmissionManager::WaitForIdle()
{
    PHX_CORE_ASSERT(vulkan_backend->vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(vulkan_backend->vk_device);
}

ICommandBuffer* phx::rhi::VulkanSubmissionManager::BeginCommandBuffer()
{
    return nullptr;
}
