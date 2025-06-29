#pragma once

#include <PhxRhi/RHICommon.h>

#include <volk.h>
#include <array>
#include <mutex>

namespace phx::RHI::vk
{
	template<size_t TBufferPoolSize>
	struct FrameCommandBufferPool
	{
		VkCommandPool vk_cmd_pool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, TBufferPoolSize> vk_cmd_buffers;
		std::atomic<uint32_t> next_buffer_index = 0;
	};

	struct AsyncCommandBuffer
	{
		VkCommandBuffer vk_buffer = VK_NULL_HANDLE;
		VkFence         vk_fence = VK_NULL_HANDLE;
	};

	template<size_t TBufferPoolSize>
	struct AsyncCommandPool
	{
		VkCommandPool vk_cmd_pool = VK_NULL_HANDLE;
		std::array<AsyncCommandBuffer, TBufferPoolSize> cmd_buffers;

		std::vector<uint16_t> free_indices;
		std::mutex pool_mutex;
	};


	template<
		typename TFramesInFlight,
		size_t TFrameBuffersPerPool,
		size_t TAsyncBuffersPerPool>
	class CommandBufferAllocator_VK
	{
	public:
		CommandBufferAllocator_VK() = default;
		~CommandBufferAllocator_VK() = default;

		void Initialize(VkDevice device, EnumArray<uint32_t, CommandQueueType> const& queue_family_indices);
		void Shutdown();

		void Recycle(uint64_t current_frame);

		void AcquireFrameCommandBuffer(CommandQueueType queue_type, uint64_t fame_index);
		void AcquireAsyncCommandBuffer(CommandQueueType queue_type);
		void ReleaseAsyncCommandBuffers(phx::Span<CommandBufferHandle> handles);

		VkCommandBuffer GetVkCommandBuffer(CommandBufferHandle handle, uint32_t frameIndex);
		VkFence GetVkFenceForAsync(CommandBufferHandle handle);

	private:
		void AllocateCommandBuffers(VkCommandPool pool, VkCommandBuffer* pCommandBuffers, uint32_t count);

	private:
		VkDevice m_vk_logical_device = VK_NULL_HANDLE;
		// We use our template parameters to define the size of our std::array members.
		std::array<EnumArray<FrameCommandBufferPool<TFrameBuffersPerPool>, CommandQueueType>, TFramesInFlight> m_frame_pools;
		EnumArray<AsyncCommandPool<TAsyncBuffersPerPool>, CommandQueueType> m_async_pools;
	};

	template<typename TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::Initialize(VkDevice device, EnumArray<uint32_t, CommandQueueType> const& queue_family_indices)
	{
		m_vk_logical_device = device;

		// --- Initialize Frame Pools ---
		for (uint32_t i = 0; i < TFramesInFlight; ++i)
		{
			for (uint32_t iQueue = 0; iQueue < static_cast<uint32_t>(CommandQueueType::Count); iQueue++)
			{
				auto& frame_pool = m_frame_pools[i][iQueue];
				VkCommandPoolCreateInfo pool_info = {};
				pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				pool_info.queueFamilyIndex = queue_family_indices[iQueue];
				pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

				vkCreateCommandPool(m_vk_logical_device, &pool_info, nullptr, &frame_pool.vk_cmd_pool);
				AllocateCommandBuffers(frame_pool.vk_cmd_pool, frame_pool.vk_cmd_buffers.data(), TFrameBuffersPerPool);
			}
		}

		// --- Initialize Async Pools ---
		for (uint32_t i = 0; i < static_cast<uint32_t>(CommandQueueType::Count); i++)
		{
			auto& async_pool = m_async_pools[i];

			VkCommandPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.queueFamilyIndex = queue_family_indices[iQueue];
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			vkCreateCommandPool(m_vk_logical_device, &pool_info, nullptr, &async_pool.vk_cmd_pool);

			// Allocate raw handles first
			std::array<VkCommandBuffer, TAsyncBuffersPerPool> raw_buffers;
			AllocateCommandBuffers(async_pool.vk_cmd_pool, raw_buffers.data(), TAsyncBuffersPerPool);

			// Populate our structures and create fences
			async_pool.freeIndices.reserve(TAsyncBuffersPerPool);
			for (uint16_t j = 0; j < TAsyncBuffersPerPool; ++j)
			{
				async_pool.cmd_buffers[j].vk_cmd_buffer = raw_buffers[j];

				VkFenceCreateInfo fence_info = {};
				fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; 
				fence_info.pNext = nullptr;
				fence_info.flags =  VK_FENCE_CREATE_SIGNALED_BIT;

				vkCreateFence(m_vk_logical_device, &fence_info, nullptr, &pool_info.cmd_buffers[j].vk_fence);
				async_pool.free_indices.push_back(j);
			}
		}
	}

	template<typename TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::Shutdown()
	{
	}

	template<typename TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::AllocateCommandBuffers(VkCommandPool pool, VkCommandBuffer* pCommandBuffers, uint32_t count)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = pool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = count;

		if (vkAllocateCommandBuffers(m_vk_logical_device, &allocInfo, pCommandBuffers) != VK_SUCCESS) 
		{
			PHX_CORE_ERROR("Failed to allocate command buffers!");
		}
	}
}