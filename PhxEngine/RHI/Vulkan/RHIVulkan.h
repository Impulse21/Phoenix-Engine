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
        VkInstance vk_instance                      = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger    = VK_NULL_HANDLE;

        VkPhysicalDevice vk_physical_device         = VK_NULL_HANDLE;

        VkDevice    vk_device                       = VK_NULL_HANDLE;
        VmaAllocator vma_allocator                  = VK_NULL_HANDLE;
    };

    inline static VulkanContext g_context;
}


#define vulkan_check(call)                                          \
    do {                                                            \
        VkResult _vk_res = (call);                                  \
        if (_vk_res != VK_SUCCESS)                                  \
        {                                                           \
            PHX_LOG_ERROR(Log::Channels::RHI,                       \
                "Vulkan call failed: {} = {}",                      \
                #call, static_cast<int>(_vk_res));                  \
            PHX_ASSERT(false);                                      \
            std::abort();                                           \
        }                                                           \
    } while(0)