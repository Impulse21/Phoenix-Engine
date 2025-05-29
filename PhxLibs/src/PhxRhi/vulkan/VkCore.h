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
	constexpr size_t cMaxInflightFrames = 2;
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

		VkExtent2D WindowExtents;

		VkSwapchainKHR Swapchain;
		VkFormat SwachainImageFormat;
		phx::Array<VkImage> SwapchainImages;
		phx::Array<VkImageView> SwapchainImageViews;
		uint32_t SwapchainImageIndex = ~0u;

		rhi::DeviceCapability Capabilities;

		struct FrameData
		{
			VkSemaphore PresentSemaphore;
			VkSemaphore RenderSemaphore;
			VkFence RenderFence;
		};

		phx::FixedArray<FrameData, cMaxInflightFrames> Frames;
		
		size_t FrameNumber = 0;
		FrameData& GetCurrentFrame() { return Frames[FrameNumber % cMaxInflightFrames]; }
		FrameData& GetLastFrame() { return Frames[FrameNumber - 1 % cMaxInflightFrames]; }
	};

	extern VkContext g_VkContext;
}