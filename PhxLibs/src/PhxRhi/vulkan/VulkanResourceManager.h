#pragma once

#include <PhxCore/Pool.h>
#include <PhxRhi/IResourceManager.h>

#include "VulkanTypes.h"

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanGpuAllocator;

	struct VulkanResourceManager final : public IResourceManager
	{
		VulkanBackend* vulkan_backend = nullptr;
		VulkanGpuAllocator* vulkan_allocator = nullptr;

		uint64_t frame_number = 0;
		phx::PagedPool<rhi::Swapchain, VulkanSwapchain> swapchain_pool;
		phx::PagedPool<rhi::Buffer, VulkanBuffer> buffer_pool;

		struct DeferredItem
		{
			uint64_t frame;
			std::function<void()> deferred_func;
		};
		std::deque<DeferredItem> deferred_queue;

		// -- Interface implementation ---
		bool Initialize() override;
		void Shutdown() override;

		//-- Swapchain ---
		SwapchainHandle CreateSwapchain(const SwapchainDesc & desc) override;
		void DeleteSwapchain(SwapchainHandle handle) override;
		TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle) override;
		void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height) override;

		// -- Buffer ---
		BufferHandle CreateBuffer(const BufferDescriptor& desc, const void* initial_data = nullptr) override;
		void DeleteBuffer(BufferHandle handle) override;

		// -- Textures ---
		TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initial_data = nullptr) override;
		void DeleteTexture(TextureHandle handle) override;

		// -- Pipeline States ---
		PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc) override;
		void DeletePipeline(PipelineStateHandle handle) override;


		void RunGarbageCollection(uint64_t completed_frame);

		void EnqueueDelete(DeferredItem&& item)
		{
			deferred_queue.emplace_back(std::forward<DeferredItem>(item));
		}


		VulkanResourceManager(VulkanBackend* vulkan_backend, VulkanGpuAllocator* vulkan_allocator);
		~VulkanResourceManager() override = default;

		VulkanResourceManager(const VulkanResourceManager&) = delete;

	private:
		int CreateSubResource(VulkanBuffer& buffer, BufferDescriptor const& desc, SubresouceType subresource_type, size_t offset, size_t size = ~0u);
	};
}

