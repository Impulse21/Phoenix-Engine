#include "PhxEngine/PhxEngine_pch.h"
#include "StandardAsyncIOSystem.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>

void phx::StandardAsyncIOSystem::Initialize()
{
    m_shutdown = false;
	(void)m_vfs;
    JobSystem::SubmitJob([this](JobContext const&) {
        this->StreamingThreadLoop();
    }, JobSystem::Type::Streaming); // Target your dedicated streaming thread)
}

void phx::StandardAsyncIOSystem::Shutdown()
{
    {
        std::scoped_lock lock(m_queueMutex);
        m_shutdown = true;
    }

    m_cv.notify_one(); // Wake up the streaming thread to exit
}

void phx::StandardAsyncIOSystem::QueueRead(data::AsyncReadRequest&& request)
{
    {
        std::scoped_lock lock(m_queueMutex);
        m_requestQueue.push_back(std::move(request));
    }

    m_cv.notify_one(); // Signal the streaming thread that new work is available
}

void phx::StandardAsyncIOSystem::Tick(float /*delta_time*/)
{
}

void phx::StandardAsyncIOSystem::StreamingThreadLoop()
{
	while (true)
	{
		data::AsyncReadRequest currentRequest;
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);

			m_cv.wait(lock, [this] { return m_shutdown || !m_requestQueue.empty(); });

			if (m_shutdown && m_requestQueue.empty())
			{
				break; // Exit loop if shutdown and queue is empty
			}
			if (m_requestQueue.empty()) 
			{
				continue;
			}

			currentRequest = std::move(m_requestQueue.front());
			m_requestQueue.pop_front();
		} // Mutex is released here

		ProcessReadRequest(currentRequest);
	}

	PHX_CORE_INFO("AsyncIOManager: Streaming thread shutting down.");
}

void phx::StandardAsyncIOSystem::ProcessReadRequest(data::AsyncReadRequest& /*request*/)
{
}
