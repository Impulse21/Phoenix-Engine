#pragma once


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <vk_mem_alloc.h>

namespace phx::rhi::vulkan
{

}


#define vulkan_check(call) [&]() { VkResult res = call; PHX_ASSERT(res >= VK_SUCCESS); return res; }()