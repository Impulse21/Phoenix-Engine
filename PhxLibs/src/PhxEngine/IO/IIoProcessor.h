#pragma once

#include <PhxEngine/StreamingDefintions.h>

namespace phx
{
	class IIoProcessor
	{
	public: 
		virtual ~IIoProcessor() = default;

		virtual void ProcessRequest(StreamingRequest&& request) = 0;

		virtual void SubmitBatchedWork(IAllocator* frame_allocator, rhi::ISubmissionManager* submission_manager) = 0;
		virtual void PullCompletions(rhi::ISubmissionManager* submission_manager) = 0;
	};
}