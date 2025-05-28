#pragma once

#define USE_PHX_ALLOCATOR 0

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "volk.h"
// Exclude if Volk is being used.
//#include <vulkan/vulkan.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <PhxRhi/RHITypes.h>

namespace phx::rhi::vk
{
	struct VkContext
	{
		VkInstance Instance;
		VkSurfaceKHR Surface;
		VkPhysicalDevice ChoosenPhysicalDevice;
		VkPhysicalDeviceProperties PhysicalDeviceProperties;
		VkDevice Device;

		VmaAllocator VmaAllocator;

#if USE_PHX_ALLOCATOR
		VkAllocationCallbacks AllocCallbacks;
#endif

		VkQueue GfxQueue;
		uint32_t GfxQueueFamily;

		VkQueue TransferQueue;
		uint32_t TransferQueueFamily;

		VkQueue ComputeQueue;
		uint32_t ComputeQueueFamily;


		rhi::DeviceCapability Capabilities;
	};

	extern VkContext g_VkContext;
}