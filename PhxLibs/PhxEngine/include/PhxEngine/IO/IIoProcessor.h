#pragma once

#include <PhxEngine/StreamingDefintions.h>

namespace phx
{
	class IAllocator;

	class IIoProcessor
	{
	public: 
		virtual ~IIoProcessor() = default;

		virtual void ProcessRequest(StreamingRequest&& request) = 0;

		virtual void SubmitBatchedWork(IAllocator* frame_allocator) = 0;
		virtual void PullCompletions() = 0;
	};
}