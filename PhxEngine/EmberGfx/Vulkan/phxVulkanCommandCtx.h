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

		void SetPipelineState(PipelineStateHandle pipelineState) override;
		void SetViewports(Span<Viewport> viewports) override;
		void SetScissors(Span<Rect> rects) override;

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) override;
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) override;

		void SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize) override;
		void SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat) override;
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