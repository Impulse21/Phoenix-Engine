#pragma once

#include <PhxEngine/StreamingDefintions.h>

namespace phx
{
	class IIoProcessor
	{
	public: 
		virtual ~IIoProcessor() = default;

		virtual void ProcessRequest(StreamingRequest&& request) = 0;

		virtual void SubmitBatchedWork() = 0;
		virtual void PullCompletions() = 0;
	};
}