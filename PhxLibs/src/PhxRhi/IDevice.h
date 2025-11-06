#pragma once

#include "RHICommon.h"

namespace phx::rhi
{

	struct Descriptor
	{
		uint32_t MaxNumTextures = 1000;
		uint32_t MaxNumGpuBuffers = 1000;
		uint32_t MaxNumPipelineStates = 1000;

	};

	class ICommandBuffer;
	class IResourceManager;
	class IGpuMemoryAllocator;

	struct FenceHandle
	{
		uint64_t cpu_fence_value = 0;
	};

	class IDevice
	{
	public:
		inline static IDevice* Ptr = nullptr;

	public:
		virtual ~IDevice() = default;
		virtual bool Initialize(Descriptor const& desc) = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame(SwapchainHandle swapChain) = 0;
		virtual void EndFrame(Span<ICommandBuffer*> cmd_buffers, SwapchainHandle  swapChain) = 0;
		virtual void WaitForIdle() = 0;

		virtual SwapchainHandle CreateSwapchain(const SwapchainDesc& desc, void* window_handle) = 0;
		virtual void DestroySwapchain(SwapchainHandle handle) = 0;
		virtual TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) = 0;
		virtual void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) = 0;

		virtual ICommandBuffer* BeginCommandBuffer() = 0;
		virtual FenceHandle Submit(Span<ICommandBuffer*> cmd_buffers) = 0;

		// -- Accessors ---
	public:
		virtual ShaderFormat GetShaderFormat() = 0;
		virtual GfxBackend GetBackend() const = 0;
		virtual IResourceManager* GetResourceManager() = 0;
		virtual IGpuMemoryAllocator* GetGpuMemoryAllocator() = 0;

	};
}