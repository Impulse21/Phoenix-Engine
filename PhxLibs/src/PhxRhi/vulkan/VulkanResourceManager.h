#pragma once

#include <PhxCore/Pool.h>
#include <PhxRhi/IResourceManager.h>
#include "VulkanTypes.h"
namespace phx::rhi
{
	struct VulkanDevice;

	class VulkanResourceManager final : public IResourceManager
	{
	public:
		VulkanResourceManager(VulkanDevice* vulkan_device);
		~VulkanResourceManager() override = default;

		VulkanResourceManager(const VulkanResourceManager&) = delete;
	public:
		void Initialize();
		void Shutdown();

	public:
		BufferHandle CreateBuffer(const BufferDescriptor& desc, const void* initialData = nullptr) override;
		void DeleteBuffer(BufferHandle handle) override;

		// -- Textures ---
		TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr) override;
		void DeleteTexture(TextureHandle handle) override;

		// -- Pipeline States ---
		PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc) override;
		void DeletePipeline(PipelineStateHandle handle) override;


		struct DeferredItem
		{
			uint64_t frame;
			std::function<void()> deferred_func;
		};

		static void ProcessDeletionQueue(uint64_t completed_frame)
		{
			while (!VkContext::deferred_queue.empty())
			{
				DeferredItem& DeferredItem = VkContext::deferred_queue.front();
				if (DeferredItem.frame + kBufferCount < completed_frame)
				{
					DeferredItem.deferred_func();
					VkContext::deferred_queue.pop_front();
				}
				else
				{
					break;
				}
			}
		}

		static void EnqueueDelete(DeferredItem&& item)
		{
			deferred_queue.emplace_back(std::forward<DeferredItem>(item));
		}

	private:
		VulkanDevice* m_vulkan_device = nullptr;
		phx::PagedPool<rhi::GpuBuffer, Buffer_VK> buffer_pool;

		inline static std::deque<DeferredItem> deferred_queue;
	};
}

