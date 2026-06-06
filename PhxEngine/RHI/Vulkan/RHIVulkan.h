#pragma once


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <vk_mem_alloc.h>

namespace phx::rhi::vulkan
{
    struct VulkanContext
    {
        VkInstance vk_instance;
        VkPhysicalDevice vk_physical_device;

        VkDevice device;
        VmaAllocator allocator;
    };

    inline static VulkanContext g_context;
}


#define vulkan_check(call) \
    do { \
        VkResult res = (call); \
        PHX_ASSERT(res == VK_SUCCESS); \
    } while(0)