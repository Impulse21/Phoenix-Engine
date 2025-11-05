#pragma once

#include <PhxEngine/StreamingDefintions.h>

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

		virtual void SubmitBatchedWork() = 0;
		virtual void PollGpuCompletions() = 0;
	};
}