#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanDevice.h"


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle
#endif


#define VOLK_IMPLEMENTATION
#include "volk.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "PhxCore/Math.h"

#ifdef PHX_PLATFORM_WINDOWS
extern HINSTANCE g_hInstance;
#endif

#define LOG_AND_SHUTDOWN_POOL(x) if (!x.IsEmpty()) PHX_CORE_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Shutdown();

phx::rhi::VulkanDevice::VulkanDevice()
	: resource_manager(this)
	, gpu_memory_allocator(this)
{
}

bool phx::rhi::VulkanDevice::Initialize(Descriptor const& desc)
{

	return false;
}
