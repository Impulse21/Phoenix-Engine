#pragma once

#include <PhxCore/Pool.h>
#include <PhxRhi/IResourceManager.h>

#include "VulkanTypes.h"

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanGpuAllocator;

	struct DeferredCallbackQueue
	{
		struct DeferredItem
		{
			uint64_t frame;
			std::function<void()> deferred_func;
		};

		std::deque<DeferredItem> queue;

		void Flush(uint64_t completed_frame = ~0u)
		{
			while (!queue.empty())
			{
				DeferredItem& deferred_item = queue.front();
				if (deferred_item.frame + kBufferCount < completed_frame)
				{
					deferred_item.deferred_func();
					queue.pop_front();
				}
				else
				{
					break;
				}
			}
		}

		void EnqueueDelete(DeferredItem&& item)
		{
			queue.emplace_back(std::forward<DeferredItem>(item));
		}
	};

	struct VulkanResourceManager final : public IResourceManager
	{
		VulkanBackend* vulkan_backend = nullptr;
		VulkanGpuAllocator* vulkan_allocator = nullptr;

		uint64_t frame_number = 0;
		phx::PagedPool<rhi::Swapchain, VulkanSwapchainFrame, VulkanSwapchain> swapchain_pool;
		phx::PagedPool<rhi::Buffer, VulkanBuffer> buffer_pool;

		DeferredCallbackQueue deferred_delete_queue;

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
		void DeleteBufferImmediate(BufferHandle handle);

		// -- Textures ---
		TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initial_data = nullptr) override;
		void DeleteTexture(TextureHandle handle) override;

		// -- Pipeline States ---
		PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc) override;
		void DeletePipeline(PipelineStateHandle handle) override;

		void RunGarbageCollection(uint64_t completed_frame);

		VulkanResourceManager(VulkanBackend* vulkan_backend, VulkanGpuAllocator* vulkan_allocator);
		~VulkanResourceManager() override = default;

		VulkanResourceManager(const VulkanResourceManager&) = delete;

	private:
		int CreateSubResource(VulkanBuffer& buffer, BufferDescriptor const& desc, SubresouceType subresource_type, size_t offset, size_t size = ~0u);
	};
}

