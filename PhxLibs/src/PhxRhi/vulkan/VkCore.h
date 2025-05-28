#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <PhxRhi/RHITypes.h>

namespace phx::rhi::vk
{
	struct VkContext
	{
		vkb::Instance Instance;
		vkb::Device Device;

		VkQueue GfxQueue;
		VkQueue TransferQueue;
		VkQueue ComputeQueue;


		rhi::DeviceCapability Capabilities;
	};

	extern VkContext g_VkContext;
}