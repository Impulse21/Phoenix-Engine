#pragma once


#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <PhxRhi/RHITypes.h>

namespace phx::rhi::vk
{
	struct VkContext
	{
		VkInstance Instance;
		VkSurfaceKHR Surface;
		VkPhysicalDevice ChoosenGpu;
		VkDevice Device;


		VkQueue GfxQueue;
		VkQueue TransferQueue;
		VkQueue ComputeQueue;


		rhi::DeviceCapability Capabilities;
	};

	extern VkContext g_VkContext;
}