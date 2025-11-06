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

	class VulkanDevice final : public IDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice() override = default;	

		VulkanDevice(const VulkanDevice&) = delete;

	public:
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
	public:
		ShaderFormat GetShaderFormat() override { return ShaderFormat::Spirv; }
		GfxBackend GetBackend() const { return GfxBackend::Vulkan; }

		IResourceManager* GetResourceManager() override { return &m_resource_manager; }
		IGpuMemoryAllocator* GetGpuMemoryAllocator() override { return &m_gpu_memory_allocator; }

	private:
		VulkanResourceManager m_resource_manager;
		VulkanGpuMemoryAllocator m_gpu_memory_allocator;

		Descriptor m_desc;
		
		inline static size_t frame_number = 0;
		inline static DeviceCapability capabilities = {};

		// -- VK Core ---
		VkInstance m_vk_instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_vk_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_vk_chosen_physical_device = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures m_vk_physical_device_features = {};

		VkPhysicalDeviceFeatures2 m_vk_features2 = {};
		VkPhysicalDeviceVulkan11Features m_vk_features_1_1 = {};
		VkPhysicalDeviceVulkan12Features m_vk_features_1_2 = {};
		VkPhysicalDeviceVulkan13Features m_vk_features_1_3 = {};

		VkPhysicalDeviceProperties2 m_vk_physical_device_properties = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT m_vk_descriptor_buffer_properties = {};
		VkDeviceSize m_vk_rebar_heap_size = 0;

		VkDevice m_vk_device = VK_NULL_HANDLE;

		VkDebugUtilsMessengerEXT m_vk_debug_messenger = VK_NULL_HANDLE;
		vkb::Instance m_vkb_instance; // vkb::Instance manages VkInstance and debug messenger

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

		Queue m_queue_gfx = {};
		Queue m_queue_compute = {};
		Queue m_queue_transfer = {};

	};
}

