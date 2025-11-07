#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanSubmissionManager.h"


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#endif

#include "volk.h"

using namespace phx;
using namespace phx::rhi;



SwapchainHandle phx::rhi::VulkanSubmissionManager::CreateSwapchain(const SwapchainDesc& desc, void* window_handle)
{
    PHX_PROFILE_SECTION("Vulkan::CreateSwapchain");
    vkb::SwapchainBuilder swapchain_builder(VkContext::vk_chosen_physical_device, VkContext::vk_device, VkContext::vk_surface); // Renamed to snake_case

    auto swap_ret = swapchain_builder
        .set_desired_format({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(desc.SwapChainDesc.Width, desc.SwapChainDesc.Height)
        .set_old_swapchain(VK_NULL_HANDLE) // For initial creation
        .build();

    if (!swap_ret)
    {
        PHX_CORE_ERROR("[RHI] Failed to create swapchain: {0}", swap_ret.error().message());
        // This is a critical failure during init
        return;
    }

    vkb::Swapchain vkb_swapchain = swap_ret.value(); // Renamed to snake_case
    VkContext::vk_swapchain = vkb_swapchain.swapchain;
    VkContext::vk_swapchain_image_format = vkb_swapchain.image_format;
    VkContext::vk_swapchain_extent = vkb_swapchain.extent;

    VkContext::vk_swapchain_images = vkb_swapchain.get_images().value();
    VkContext::vk_swapchain_image_views = vkb_swapchain.get_image_views().value();

    PHX_CORE_INFO(
        "[RHI] Swapchain Initialized. Extent: {0}x{1}, Format: {2}, Images: {3}",
        VkContext::vk_swapchain_extent.width,
        VkContext::vk_swapchain_extent.height,
        "", // Placeholder for format name conversion
        VkContext::vk_swapchain_images.size());
}

void phx::rhi::VulkanSubmissionManager::DestroySwapchain(SwapchainHandle handle)
{
    if (VkContext::vk_device == VK_NULL_HANDLE)
        return;

    for (auto image_view : VkContext::vk_swapchain_image_views) // Renamed to snake_case
    {
        if (image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VkContext::vk_device, image_view, GetVkAllocationCallbacks());
        }
    }
    VkContext::vk_swapchain_image_views.clear();
    VkContext::vk_swapchain_images.clear();

    if (VkContext::vk_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VkContext::vk_device, VkContext::vk_swapchain, GetVkAllocationCallbacks());
        VkContext::vk_swapchain = VK_NULL_HANDLE;
    }
}


void CreateFrameSyncObjects()
{
    PHX_PROFILE_SECTION("Vulkan::CreateFrameSyncObjects");
    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; // Renamed to snake_case
    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; // Renamed to snake_case
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < cMaxInflightFrames; ++i)
    {
        VkResult result = vkCreateSemaphore(VkContext::vk_device, &semaphore_info, GetVkAllocationCallbacks(), &VkContext::frames[i].PresentSemaphore);
        PHX_CORE_ASSERT(result == VK_SUCCESS);

        result = vkCreateSemaphore(VkContext::vk_device, &semaphore_info, GetVkAllocationCallbacks(), &VkContext::frames[i].RenderSemaphore);
        PHX_CORE_ASSERT(result == VK_SUCCESS);

        result = vkCreateFence(VkContext::vk_device, &fence_info, GetVkAllocationCallbacks(), &VkContext::frames[i].RenderFence);
        PHX_CORE_ASSERT(result == VK_SUCCESS);
    }

    PHX_CORE_INFO("[RHI] Frame synchronization primitives created.");
}

void DestroyFrameSyncObjects()
{
    if (VkContext::vk_device == VK_NULL_HANDLE) return;
    for (size_t i = 0; i < cMaxInflightFrames; ++i)
    {
        if (VkContext::frames[i].RenderFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(VkContext::vk_device, VkContext::frames[i].RenderFence, GetVkAllocationCallbacks());
            VkContext::frames[i].RenderFence = VK_NULL_HANDLE;
        }
        if (VkContext::frames[i].RenderSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(VkContext::vk_device, VkContext::frames[i].RenderSemaphore, GetVkAllocationCallbacks());
            VkContext::frames[i].RenderSemaphore = VK_NULL_HANDLE;
        }
        if (VkContext::frames[i].PresentSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(VkContext::vk_device, VkContext::frames[i].PresentSemaphore, GetVkAllocationCallbacks());
            VkContext::frames[i].PresentSemaphore = VK_NULL_HANDLE;
        }
    }
    PHX_CORE_INFO("[RHI] Frame synchronization primitives destroyed.");
}

void CreateCommandPools()
{
    PHX_PROFILE_SECTION("Vulkan::CreateCommandPools");

    VkCommandPoolCreateInfo pool_info = {}; // Renamed to snake_case
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = VkContext::queue_gfx.vk_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult result = vkCreateCommandPool(VkContext::vk_device, &pool_info, GetVkAllocationCallbacks(), &VkContext::vk_graphics_command_pool);

    PHX_CORE_ASSERT(result == VK_SUCCESS);
    PHX_CORE_INFO("[RHI] Graphics Command Pool created.");
    // Create other command pools (compute, transfer) if needed
}

void DestroyCommandPools()
{
    if (VkContext::vk_graphics_command_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VkContext::vk_device, VkContext::vk_graphics_command_pool, GetVkAllocationCallbacks());
        VkContext::vk_graphics_command_pool = VK_NULL_HANDLE;
        PHX_CORE_INFO("[RHI] Graphics Command Pool destroyed.");
    }
}

void phx::rhi::VulkanSubmissionManager::WaitForIdle()
{
    PHX_CORE_ASSERT(VkContext::is_initialized && VkContext::vk_device != VK_NULL_HANDLE);
    vkDeviceWaitIdle(VkContext::vk_device);
}
