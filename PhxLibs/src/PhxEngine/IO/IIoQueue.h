#pragma once

#include <PhxEngine/StreamingDefintions.h>

namespace phx::rhi
{
	class ISubmissionManager;
}

namespace phx
{
	class IIoQueue
	{
	public:
		inline static IIoQueue* Ptr = nullptr;
	public:
		virtual ~IIoQueue() = default;

		virtual void Initialize(bool use_dstorage) = 0;
		virtual void Shutdown() = 0;

		virtual void Submit(StreamingRequest&& request) = 0;

		virtual void SubmitBatchedWork(IAllocator* frame_allocator, rhi::ISubmissionManager* submission_manager) = 0;
		virtual void PollGpuCompletions(rhi::ISubmissionManager* submission_manager) = 0;
	};
}