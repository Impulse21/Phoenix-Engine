#include "PhxResource/PhxResource_pch.h"
#include "AsyncLoader.h"

#include <PhxResource/ResourceManager.h>

phx::AsyncLoader::AsyncLoader() = default;

phx::AsyncLoader::~AsyncLoader()
{
	Stop();
}

void phx::AsyncLoader::Start()
{
	m_running = true;
	m_thread = std::thread(&AsyncLoader::ThreadLoop, this);
}

void phx::AsyncLoader::Stop()
{
	if (!m_running)
		return;

	m_running = false;
	m_wake_signal.notify_all(); // Wake up so it can exit
	if (m_thread.joinable())
		m_thread.join();
}

void phx::AsyncLoader::QueueRequest(const LoadRequest& req)
{
	{
		std::scoped_lock lock(m_queue_mutex);
		m_pending_requests.push_back(req);
	}

	m_wake_signal.notify_one();
}

void phx::AsyncLoader::CancelRequest(GenericHandle)
{
	PHX_CORE_ASSERT(false, "Not implemented");
}

void phx::AsyncLoader::ThreadLoop()
{
    std::vector<LoadRequest> incoming;
    incoming.reserve(16);

    while (m_running)
    {
        {
            std::unique_lock lock(m_queue_mutex);
            if (m_pending_requests.empty() && m_active_jobs.empty())
            {
                m_wake_signal.wait(lock, [this] {
                    return !m_pending_requests.empty() || !m_running;
                });
            }

            if (!m_running) 
                break;

            if (!m_pending_requests.empty())
            {
                incoming.swap(m_pending_requests);
            }
        }

        for (const auto& req : incoming)
        {
            ActiveJob job = {
                .loader = req.loader_interface,
                .ctx {
                    .handle = req.handle,
                    .virtual_file_path = req.virtual_path,
                    .state_index = 0xFF,
                },
            };

            // Important: Set global state so Game Logic knows it's started
            ResourceManager::SetState(req.handle, ResourceState::Loading);

            m_active_jobs.push_back(std::move(job));
        }

        incoming.clear();

        bool did_work = false;
        for (size_t i = 0; i < m_active_jobs.size(); )
        {
            ActiveJob& job = m_active_jobs[i];
            LoaderStepResult result = job.loader->Step(job.ctx);

            if (result == LoaderStepResult::Done)
            {
                ResourceManager::PushToGpuTransitionQueue(job.ctx.handle);

                if (i != m_active_jobs.size() - 1)
                    m_active_jobs[i] = std::move(m_active_jobs.back());

                m_active_jobs.pop_back();
                did_work = true;
            }
            else if (result == LoaderStepResult::Error)
            {
                ResourceManager::SetState(job.ctx.handle, ResourceState::Error);

                if (i != m_active_jobs.size() - 1)
                {
                    m_active_jobs[i] = std::move(m_active_jobs.back());
                }
                m_active_jobs.pop_back();
                did_work = true;
            }
            else if (result == LoaderStepResult::Continue)
            {
                did_work = true;
                ++i;
            }
            else
            {
                ++i;
            }
        }

        // If we did work (CPU processing), we loop fast.
        // If everyone yielded (waiting for IO), we sleep briefly to let IO thread work.
        if (!did_work && !m_active_jobs.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

