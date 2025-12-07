#pragma once

#include "IIoQueue.h"
#include "IIoProcessor.h"

#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{
	class IoQueue final : public IIoQueue
	{
	public:
		IoQueue() = default;
		~IoQueue() = default;

		void Initialize(bool use_dstroage) override;
		void Shutdown() override;

		void Submit(StreamingRequest&& request) override;
		void SubmitBatchedWork(IAllocator* frame_allocator) override;
		void PollGpuCompletions() override;

	private:
		void StreamingThreadLoop();

	private:
		std::unique_ptr<IIoProcessor> m_io_processor;
		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;

		std::deque<StreamingRequest> m_request_queue;
		std::mutex m_queue_mutex;
	};
}