#pragma once

#include <PhxEngine/StreamingDefintions.h>

namespace phx
{
	class IAllocator;
	struct IOTicket
	{
		uint64_t id = 0;
		bool IsValid() const { return id != 0; }
		bool operator==(const IOTicket& other) const { return id == other.id; }
	};

	class IIoQueue
	{
	public:
		inline static IIoQueue* Ptr = nullptr;
	public:
		virtual ~IIoQueue() = default;

		virtual void Initialize(bool use_dstorage) = 0;
		virtual void Shutdown() = 0;

		virtual IOTicket Submit(StreamingRequest&& request) = 0;

		virtual void SubmitBatchedWork(IAllocator* frame_allocator) = 0;
		virtual void PollGpuCompletions() = 0;

		virtual bool IsComplete(IOTicket ticket) = 0;
		virtual StreamingResult GetResult(IOTicket ticket) = 0;
	};
}