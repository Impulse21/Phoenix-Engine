#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include "EmberGfx/phxCommandCtxInterface.h"

namespace phx::gfx::platform
{
	class VulkanGpuDevice;
	class  CommandCtx_Vulkan final : public ICommandCtx
	{
	public:
		// -- Command Interface ----
		void RenderPassBegin() override;
		void RenderPassEnd() override;

	public:
		EnumArray<VkCommandBuffer, CommandQueueType>  CmdBufferVk[kBufferCount];
		EnumArray<VkCommandPool, CommandQueueType> CmdBufferPoolVk[kBufferCount];

		CommandQueueType QueueType = {};
		uint32_t Id = 0;

		std::vector<std::pair<CommandQueueType, VkSemaphore>> WaitQueues;
		std::vector<VkSemaphore> Waits;
		std::vector<VkSemaphore> Signals;

		uint32_t CurrentBufferIndex = 0;

		VulkanGpuDevice* GpuDevice = nullptr;
		std::vector<VkImageMemoryBarrier2> BarriersRenderPassEnd;

		void Reset(uint32_t bufferIndex)
		{
			CurrentBufferIndex = bufferIndex;
			Signals.clear();
			Waits.clear();
			WaitQueues.clear();
		}

		VkCommandBuffer GetVkCommandBuffer()
		{
			return CmdBufferVk[CurrentBufferIndex][QueueType];
		}

		VkCommandPool GetVkCommandPool()
		{
			return CmdBufferPoolVk[CurrentBufferIndex][QueueType];
		}
	};
}