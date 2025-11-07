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

		// -- Interface implementation ---
		bool Initialize() override;
		void Shutdown() override;

		void BeginFrame(SwapchainHandle swapChain) override;
		void EndFrame(Span<ICommandBuffer*> cmd_buffers, SwapchainHandle swapChain) override;

		void WaitForIdle() override;

		ICommandBuffer* BeginCommandBuffer() override;

		FenceHandle Submit(Span<ICommandBuffer*> cmd_buffers) override;

		SwapchainHandle CreateSwapchain(const SwapchainDesc& desc, void* window_handle) override;
		void DestroySwapchain(SwapchainHandle handle) override;
		TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) override;
		void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) override;

		VulkanSubmissionManager(VulkanBackend* vulkan_device, VulkanResourceManager* vulkan_resource_manager);
		~VulkanSubmissionManager() override = default;

	};
}

