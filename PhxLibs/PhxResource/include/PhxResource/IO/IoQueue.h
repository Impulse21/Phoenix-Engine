#pragma once

#include <PhxResource/IO/IIoQueue.h>
#include <PhxResource/IO/IIoProcessor.h>
#include <PhxCore/TaskScheduler.h>

#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{
	class IoQueue final : public IIoQueue
	{
	public:
		IoQueue(ThreadPoolHandle thread_pool_handle)
			: m_thread_pool_handle(thread_pool_handle)
		{};
		~IoQueue() = default;

		void Initialize(bool use_dstroage) override;
		void Shutdown() override;

		IOTicket Submit(StreamingRequest&& request) override;
		void SubmitBatchedWork(IAllocator* frame_allocator) override;
		void PollGpuCompletions() override;

		bool IsComplete(IOTicket ticket) override;
		StreamingResult GetResult(IOTicket ticket) override;

	private:
		void StreamingThreadLoop();
		void OnRequestFinished(uint64_t id, const StreamingResult& result);

	private:
		ThreadPoolHandle m_thread_pool_handle;
		std::unique_ptr<IIoProcessor> m_io_processor;
		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;

		std::deque<StreamingRequest> m_request_queue;
		std::mutex m_queue_mutex;

		std::mutex m_result_mutex;
		std::atomic<uint64_t> m_next_ticket_id = 1;

		std::unordered_map<uint64_t, StreamingResult> m_completed_store;
	};
}