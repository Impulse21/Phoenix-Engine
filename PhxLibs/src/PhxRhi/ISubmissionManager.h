#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class ICommandBuffer;

	class ISubmissionManager
	{
	public:
		inline static ISubmissionManager* Ptr = nullptr;

	public:
		virtual ~ISubmissionManager() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame(SwapchainHandle swapChain) = 0;
		virtual void EndFrame(Span<ICommandBuffer*> cmd_buffers, SwapchainHandle  swapChain) = 0;
		virtual void WaitForIdle() = 0;

		virtual ICommandBuffer* BeginCommandBuffer() = 0;
		virtual FenceHandle Submit(Span<ICommandBuffer*> cmd_buffers) = 0;
		
		virtual SwapchainHandle CreateSwapchain(const SwapchainDesc& desc, void* window_handle) = 0;
		virtual void DestroySwapchain(SwapchainHandle handle) = 0;
		virtual TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) = 0;
		virtual void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) = 0;
	};
}