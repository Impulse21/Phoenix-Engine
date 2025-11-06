#pragma once

#include <PhxRhi/IDevice.h>

#include "VulkanResourceManager.h"
#include "VulkanGpuMemoryAllocator.h"

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

	struct VulkanDevice final : public IDevice
	{
	public:
		VulkanResourceManager resource_manager;
		VulkanGpuMemoryAllocator gpu_memory_allocator;

		Descriptor desc;

		inline static size_t frame_number = 0;
		inline static DeviceCapability capabilities = {};

		// -- VK Core ---
		VkInstance vk_instance = VK_NULL_HANDLE;
		VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
		VkPhysicalDevice vk_chosen_physical_device = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures vk_physical_device_features = {};

		VkPhysicalDeviceFeatures2 vk_features2 = {};
		VkPhysicalDeviceVulkan11Features vk_features_1_1 = {};
		VkPhysicalDeviceVulkan12Features vk_features_1_2 = {};
		VkPhysicalDeviceVulkan13Features vk_features_1_3 = {};

		VkPhysicalDeviceProperties2 vk_physical_device_properties = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT vk_descriptor_buffer_properties = {};
		VkDeviceSize vk_rebar_heap_size = 0;

		VkDevice vk_device = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
		vkb::Instance vkb_instance; // vkb::Instance manages VkInstance and debug messenger

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
		bool Initialize(Descriptor const& desc) override;
		void Shutdown() override;

		void BeginFrame(SwapchainHandle swapChain) override;
		void EndFrame(Span<ICommandBuffer*> cmd_buffers, SwapchainHandle  swapChain) override;
		void WaitForIdle() override;

		SwapchainHandle CreateSwapchain(const SwapchainDesc& desc, void* window_handle) override;
		void DestroySwapchain(SwapchainHandle handle) override;
		TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) override;
		void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) override;

		ICommandBuffer* BeginCommandBuffer() override;
		FenceHandle Submit(Span<ICommandBuffer*> cmd_buffers) override;

		// -- Accessors ---
		ShaderFormat GetShaderFormat() override { return ShaderFormat::Spirv; }
		GfxBackend GetBackend() const { return GfxBackend::Vulkan; }

		IResourceManager* GetResourceManager() override { return &resource_manager; }
		IGpuMemoryAllocator* GetGpuMemoryAllocator() override { return &gpu_memory_allocator; }


		VulkanDevice();
		~VulkanDevice() override = default;

		VulkanDevice(const VulkanDevice&) = delete;
	};
}

