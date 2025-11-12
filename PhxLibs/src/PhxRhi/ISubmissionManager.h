#pragma once

#include "RHICommon.h"

#include <PhxRhi/ICommandBuffer.h>
namespace phx::rhi
{
	class ISubmissionManager
	{
	public:
		inline static ISubmissionManager* Ptr = nullptr;

	public:
		virtual ~ISubmissionManager() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame(SwapchainHandle swapChain) = 0;
		virtual void EndFrame(
			SwapchainHandle swapChain,
			Span<ICommandBuffer*> graphics_buffers,
			Span<FenceHandle> wait_fences = {}) = 0;

		virtual void WaitForIdle() = 0;
		virtual bool IsFenceCompleted(FenceHandle handle) = 0;

		virtual StagingBlock RequestStagingMemory(uint32_t size, uint32_t aligmnet = 16) = 0;

		virtual ICommandBuffer* BeginCommandBuffer(CommandQueueType queue_type) = 0;
		virtual FenceHandle Submit(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences) = 0;
	};
}