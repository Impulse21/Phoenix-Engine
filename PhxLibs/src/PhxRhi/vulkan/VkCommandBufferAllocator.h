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
		size_t TFramesInFlight,
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

		CommandBufferHandle AcquireFrameCommandBuffer(CommandQueueType queue_type, uint64_t fame_index);
		CommandBufferHandle AcquireAsyncCommandBuffer(CommandQueueType queue_type);
		void ReleaseAsyncCommandBuffer(CommandBufferHandle handle);

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

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
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
			pool_info.queueFamilyIndex = queue_family_indices[i];
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

				vkCreateFence(m_vk_logical_device, &fence_info, nullptr, &async_pool.cmd_buffers[j].vk_fence);
				async_pool.free_indices.push_back(j);
			}
		}
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::Shutdown()
	{
		for (uint32_t i = 0; i < TFramesInFlight; ++i) 
		{
			for (auto& pool : m_frame_pools[i]) 
			{
				vkDestroyCommandPool(m_vk_logical_device, pool.vk_cmd_pool, nullptr);
			}
		}

		for (auto& pool : m_async_pools) 
		{
			for (auto& cmd : pool.cmd_buffers)
			{
				vkDestroyFence(m_vk_logical_device, cmd.vk_fence, nullptr);
			}
			vkDestroyCommandPool(m_vk_logical_device, pool.vk_cmd_pool, nullptr);
		}
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::Recycle(uint64_t current_frame)
	{
		for (auto& pool : m_frame_pools[current_frame % TFramesInFlight])
		{
			vkResetCommandPool(m_vk_logical_device, pool.vk_cmd_pool, 0);
			pool.next_buffer_index.store(0);
		}
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline CommandBufferHandle CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::AcquireFrameCommandBuffer(CommandQueueType queue_type, uint64_t fame_index)
	{
		auto& pool = m_frame_pools[fame_index][queue_type];
		const uint32_t buffer_index = pool.next_buffer_index.fetch_add(1);

		if (buffer_index >= TFrameBuffersPerPool)
		{
			PHX_CORE_ERROR("Exceeded available frame command buffers for queue type %d! Budget is %u.", (int)queue_type, TFrameBuffersPerPool);
			return {};
		}

		VkCommandBuffer vk_cmd_buffer = pool.vk_cmd_buffers[buffer_index];

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = nullptr;
		begin_info.pInheritanceInfo = nullptr;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(vk_cmd_buffer, &begin_info);

		CommandBufferHandle handle;
		handle.data.index = static_cast<uint16_t>(buffer_index);
		handle.data.queue_type = static_cast<uint8_t>(queue_type);
		handle.data.pool_type = static_cast<uint8_t>(PoolType::Frame);
		return handle;
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline CommandBufferHandle CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::AcquireAsyncCommandBuffer(CommandQueueType queue_type)
	{
		auto& pool = m_async_pools[queue_type];
		std::scoped_lock _(pool.pool_mutex);

		if (pool.free_indices.empty())
		{
			PHX_CORE_ERROR("No async command buffers available for queue type %d! Budget is %u.", static_cast<uint32_t>(queue_type), TAsyncBuffersPerPool);
			return CommandBufferHandle{ 0 };
		}

		uint16_t buffer_index = pool.free_indices.back();
		pool.free_indices.pop_back();

		AsyncCommandBuffer& async_cmd = pool.cmd_buffers[buffer_index];
		vkWaitForFences(m_vk_logical_device, 1, &async_cmd.vk_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_vk_logical_device, 1, &async_cmd.vk_fence);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = nullptr;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(async_cmd.vk_buffer, &begin_info);

		CommandBufferHandle handle;
		handle.data.index = buffer_index;
		handle.data.queue_type = static_cast<uint8_t>(queue_type);
		handle.data.pool_type = static_cast<uint8_t>(PoolType::Async);
		return handle;
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline void CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::ReleaseAsyncCommandBuffer(CommandBufferHandle handle)
	{
		PHX_CORE_ASSERT(handle.GetPoolType() == PoolType::Async);
		if (handle.GetPoolType() != PoolType::Async)
			return;

		auto& pool = m_async_pools[static_cast<uint32_t>(handle.GetQueueType())];
		std::scoped_lock _(pool.pool_mutex);
		pool.free_indices.push_back(handle.GetIndex());
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline VkCommandBuffer CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::GetVkCommandBuffer(CommandBufferHandle handle, uint32_t frameIndex)
	{
		if (handle.IsFrame()) {
			return m_frame_pools[frameIndex][handle.GetQueueType()].vk_cmd_buffers[handle.GetIndex()];
		}
		return m_async_pools[handle.GetQueueType()].cmd_buffers[handle.GetIndex()].vk_cmd_buffer;
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
	inline VkFence CommandBufferAllocator_VK<TFramesInFlight, TFrameBuffersPerPool, TAsyncBuffersPerPool>::GetVkFenceForAsync(CommandBufferHandle handle)
	{
		PHX_CORE_ASSERT(handle.GetPoolType() == PoolType::Async);
		if (handle.GetPoolType() != PoolType::Async)
			return VK_NULL_HANDLE;

		return m_async_pools[handle.GetQueueType()].cmd_buffers[handle.GetIndex()].vk_fence;
	}

	template<size_t TFramesInFlight, size_t TFrameBuffersPerPool, size_t TAsyncBuffersPerPool>
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