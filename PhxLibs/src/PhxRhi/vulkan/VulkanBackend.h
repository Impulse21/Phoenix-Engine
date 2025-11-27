#pragma once

#include <PhxRhi/IBackend.h>

#include "VulkanGpuAllocator.h"
#include "VulkanDescriptorSystem.h"

#ifdef PHX_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>
#include <VkBootstrap.h>

#include <deque>

namespace phx::rhi
{
	struct VulkanBackend final : public IBackend
	{
		DeviceCapability capabilities = {};

		// -- VK Core ---
		VkInstance vk_instance = VK_NULL_HANDLE;
		vkb::Instance vkb_instance; // vkb::Instance manages VkInstance and debug messenger

		void* window_handle = nullptr;
		VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

		VkPhysicalDevice vk_chosen_physical_device = VK_NULL_HANDLE;

		VkDevice vk_device = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures vk_physical_device_features = {};

		VkPhysicalDeviceFeatures2 vk_features2 = {};
		VkPhysicalDeviceVulkan11Features vk_features_1_1 = {};
		VkPhysicalDeviceVulkan12Features vk_features_1_2 = {};
		VkPhysicalDeviceVulkan13Features vk_features_1_3 = {};

		VkPhysicalDeviceProperties2 vk_physical_device_properties = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT vk_descriptor_buffer_properties = {};
		VkDeviceSize vk_rebar_heap_size = 0;

		VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;

		VulkanGpuAllocator vulkan_allocator;
		vulkan::DescriptorSystem descriptor_system;
		struct Queue
		{
			struct QueueExecutionInfo
			{
				uint64_t fence_value;
				VkCommandBuffer commad_buffer = VK_NULL_HANDLE;
				VkCommandPool command_pool = VK_NULL_HANDLE;
			};

			VkQueue vk_queue = VK_NULL_HANDLE;
			uint32_t vk_queue_family = UINT32_MAX;
			std::vector<std::pair<VkCommandBuffer, VkCommandPool>> pending_commands;
			std::deque<QueueExecutionInfo> available_commands;

			std::mutex lock = {};
			std::mutex lock_commands = {};
		};

		EnumArray<Queue, CommandQueueType> queues;

		// -- Interface Implementation ---
		bool Initialize() override;
		void Shutdown() override;

		// -- Accessors ---
		ShaderFormat GetShaderFormat() override { return ShaderFormat::Spirv; }
		GfxBackend GetBackend() const override { return GfxBackend::Vulkan; }

		VulkanBackend(void* window_handle);
		~VulkanBackend() override = default;

		VulkanBackend(const VulkanBackend&) = delete;

	private:
		bool SelectPhysicalDevice(vkb::PhysicalDevice& out_vkb_physical_device);
		bool CreateLogicalDevice(vkb::PhysicalDevice& vkb_physical_device);

#if defined(PHX_PLATFORM_WINDOWS)
		bool CreateSurface_Win32(void* window_handle);
#endif
	};
}
