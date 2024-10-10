#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include "phxVulkanManager.h"

namespace phx::gfx::platform
{
	constexpr size_t kBufferCount = 2;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> ComputeFamily;
		std::optional<uint32_t> TransferFamily;
		std::optional<uint32_t> PresentFamily;

		bool IsComplete() 
		{
			return 
				GraphicsFamily.has_value() && 
				PresentFamily.has_value() &&
				TransferFamily.has_value() &&
				ComputeFamily.has_value();
		}
	};

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct SwapChainVK
	{
		VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
		VkFormat SwapChainImageFormat;
		VkExtent2D SwapChainExtent;
	};

	class VulkanGpuDevice
	{
	public:
		void Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle = nullptr);
		void Finalize();

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSurface(void* windowHandle);
		void CreateSwapchain(SwapChainDesc const& desc);

		int32_t RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

	private:

		bool m_enableValidationLayers;
		VulkanExtManager m_extManager;
		VulkanLayerManager m_layerManager;

		VkInstance m_vkInstance;
		VkPhysicalDevice m_vkPhysicalDevice;
		VkDevice m_vkDevice;

		VkQueue m_vkQueueGfx;
		VkQueue m_vkComputeQueue;
		VkQueue m_vkTransferQueue;
		VkQueue m_vkPresentQueue;

		VkSurfaceKHR m_vkSurface;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		SwapChainVK m_swapChain;

	};
}

