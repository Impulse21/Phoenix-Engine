#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::rhi
{
	class IResourceManager
	{
	public:
		inline static IResourceManager* Ptr = nullptr;

	public:
		virtual ~IResourceManager() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		// -- Swapchain ---
		virtual SwapchainHandle CreateSwapchain(const SwapchainDesc& desc) = 0;
		virtual void DeleteSwapchain(SwapchainHandle handle) = 0;
		virtual TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) = 0;
		virtual void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) = 0;

		// -- Buffers  ---
		virtual BufferHandle CreateBuffer(const BufferDescriptor& desc, const void* initialData = nullptr) = 0;
		virtual void DeleteBuffer(BufferHandle handle) = 0;

		// -- Textures ---
		virtual TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr) = 0;
		virtual void DeleteTexture(TextureHandle handle) = 0;

		// -- Pipeline States ---
		virtual PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc) = 0;
		virtual void DeletePipeline(PipelineStateHandle handle) = 0;
	};
}