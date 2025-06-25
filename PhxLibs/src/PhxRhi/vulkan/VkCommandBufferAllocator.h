#pragma once

#include "VkRhi_Internal.h"
#include <vector>
#include <mutex>

namespace phx::RHI::vk
{
	template<size_t _MaxFramesInFlight>
	class VkCommandBufferAllocator
	{
	public:
		void Initialize(VkDevice vk_logical_device)
		{
			m_vk_logical_device = vk_logical_device;
		}

		CommandBufferHandle AquireFrameBuffer(uint64_t frame_index, CommandQueueType queue_type);
		CommandBufferHandle AquireAsyncBuffer(uint64_t frame_index, CommandQueueType queue_type);

		CommandBuffer_VK* GetVkBuffer(CommandBufferHandle handle);
		void SubmitComplete(Span<CommandBufferHandle> handles);
		void Recycle(uint64_t current_frame)

	private:
		struct FrameCommandPool
		{
			VkCommandPool vk_cmd_pool = VK_NULL_HANDLE;
			std::vector<VkCommandBuffer> buffers;
			std::deque<size_t> m_free_indices;
		};

		struct AsyncCommandPool
		{
			VkCommandPool vk_cmd_pool = VK_NULL_HANDLE;
			struct Entry
			{
				VkCommandBuffer vk_cmd_buffer = VK_NULL_HANDLE;
				VkFence fence = VK_NULL_HANDLE;
				uint16_t generation = 0;
			};

			std::vector<Entry> entries;
			std::deque<size_t> m_free_indices;
		};

		VkDevice m_vk_logical_device;
		std::vector<CommandBuffer_VK> m_command_buffers;
		std::deque<size_t> m_free_indices;


		FrameCommandPool m_frame_pools[NumCommandQueues];
		AsyncCommandPool m_async_pools[NumCommandQueues];

		VkFence m_frame_fences[_MaxFramesInFlight];
	};

	template<size_t _MaxFramesInFlight>
	inline CommandBufferHandle VkCommandBufferAllocator<_MaxFramesInFlight>::AquireFrameBuffer(uint64_t frame_index, CommandQueueType queue_type)
	{
		return CommandBufferHandle();
	}

	template<size_t _MaxFramesInFlight>
	inline CommandBufferHandle VkCommandBufferAllocator<_MaxFramesInFlight>::AquireAsyncBuffer(uint64_t frame_index, CommandQueueType queue_type)
	{
		return CommandBufferHandle();
	}
	template<size_t _MaxFramesInFlight>
	inline CommandBuffer_VK* VkCommandBufferAllocator<_MaxFramesInFlight>::GetVkBuffer(CommandBufferHandle handle)
	{
		return nullptr;
	}
	template<size_t _MaxFramesInFlight>
	inline void VkCommandBufferAllocator<_MaxFramesInFlight>::SubmitComplete(Span<CommandBufferHandle> handles)
	{
	}
}