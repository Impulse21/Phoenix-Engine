#include "PhxEngine/PhxEngine_pch.h"

#include "IoQueue.h"
#include "StandardFileProcessor.h"

#include <PhxEngine/JobSystem.h>
using namespace phx;


void phx::IoQueue::Initialize(bool /*use_dstroage*/)
{
	m_shutdown = false;
	m_io_processor = std::make_unique<StandardFileProcessor>();

	JobSystem::SubmitJobToStreaming([this](JobContext const&) {
		this->StreamingThreadLoop();
		}); // Target your dedicated streaming thread)
}

void phx::IoQueue::Shutdown()
{
	{
		std::scoped_lock lock(m_queue_mutex);
		m_shutdown = true;
	}

	m_cv.notify_one(); // Wake up the streaming thread to exit

	JobSystem::Wait(JobSystem::Type::Streaming);

	// NOTE: Shutdown() should have already been called, 
	// but this is good practice for safety.
	if (!m_shutdown)
	{
		Shutdown();
	}

}

void phx::IoQueue::Submit(StreamingRequest&& request)
{
	{
		std::scoped_lock lock(m_queue_mutex);
		request.request_id = RequestIdGenerator();
		m_request_queue.push_back(std::move(request));
	}

	m_cv.notify_one(); // Signal the streaming thread that new work is available
}

void phx::IoQueue::SubmitBatchedWork(IAllocator* frame_allocator)
{
	m_io_processor->SubmitBatchedWork(frame_allocator);
}

void phx::IoQueue::PollGpuCompletions()
{
	m_io_processor->PullCompletions();
}

void IoQueue::StreamingThreadLoop()
{
	while (true)
	{
		StreamingRequest current_request;
		{
			std::unique_lock<std::mutex> lock(m_queue_mutex);

			m_cv.wait(lock, [this] { return m_shutdown || !m_request_queue.empty(); });

			if (m_shutdown && m_request_queue.empty())
			{
				break; // Exit loop if shutdown and queue is empty
			}
			if (m_request_queue.empty())
			{
				continue;
			}

			current_request = std::move(m_request_queue.front());
			m_request_queue.pop_front();
		} // Mutex is released here

		{

			PHX_CORE_INFO("Processing Request {0}...", current_request.debug_name);

			m_io_processor->ProcessRequest(std::move(current_request));
			PHX_CORE_INFO("Request Processed {0}...", current_request.debug_name);

		} // Release m_batchMutex
	}

	PHX_CORE_INFO("AsyncIOManager: Streaming thread shutting down.");

}