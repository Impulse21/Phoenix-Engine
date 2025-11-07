#pragma once

#include <PhxRhi/ISubmissionManager.h>

#include <volk.h>

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanResourceManager;

	struct VulkanSubmissionManager : public ISubmissionManager
	{
		size_t frame_number = 0;
		VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

		VulkanSubmissionManager(VulkanBackend* vulkan_device, VulkanResourceManager* vulkan_resource_manager);
		~VulkanSubmissionManager() override = default;

	};
}

