#pragma once

#include <PhxRhi/IBackend.h>

#include "VulkanResourceManager.h"

#include <volk.h>
#include <VkBootstrap.h>

#include <deque>

namespace phx::rhi
{
	constexpr size_t kMaxNumBuffers = 4096;
	constexpr size_t kMaxNumTextures = 4096;

	constexpr size_t cMaxInflightFrames = 2;
	constexpr uint64_t kTimeoutValue = 2000000000ull; // 2 seconds
	constexpr uint32_t kMaxFrameCmds = 64;
	constexpr uint32_t kMaxAsyncCmds = 32;

	struct VulkanBackend final : public IBackend
	{
		inline static DeviceCapability capabilities = {};

		// -- VK Core ---
		VkInstance vk_instance = VK_NULL_HANDLE;
		vkb::Instance vkb_instance; // vkb::Instance manages VkInstance and debug messenger

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

		Queue queue_gfx = {};
		Queue queue_compute = {};
		Queue queue_transfer = {};

		// -- Interface Implementation ---
		bool Initialize() override;
		void Shutdown() override;

		// -- Accessors ---
		ShaderFormat GetShaderFormat() override { return ShaderFormat::Spirv; }
		GfxBackend GetBackend() const { return GfxBackend::Vulkan; }

		VulkanBackend() = default;
		~VulkanBackend() override = default;

		VulkanBackend(const VulkanBackend&) = delete;

	private:
		bool SelectPhysicalDevice(vkb::PhysicalDevice& out_vkb_physical_device);
		bool CreateLogicalDevice(vkb::PhysicalDevice& vkb_physical_device);
	};
}
