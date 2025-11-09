#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class ICommandBuffer;

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

		virtual ICommandBuffer* BeginCommandBuffer(CommandQueueType queue_type) = 0;
		virtual FenceHandle Submit(
			CommandQueueType queue_type,
			Span<ICommandBuffer*> cmd_buffers,
			Span<FenceHandle> wait_fences) = 0;
	};
}