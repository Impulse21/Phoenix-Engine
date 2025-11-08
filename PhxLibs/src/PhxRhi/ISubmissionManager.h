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
		virtual void EndFrame(Span<ICommandBuffer*> cmd_buffers, SwapchainHandle  swapChain) = 0;
		virtual void WaitForIdle() = 0;

		virtual ICommandBuffer* BeginCommandBuffer() = 0;
		virtual FenceHandle Submit(Span<ICommandBuffer*> cmd_buffers) = 0;
	};
}