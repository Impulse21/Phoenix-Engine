#include "PhxResource_pch.h"

#include <PhxResource/AsyncLoader.h>

#include <PhxResource/ResourceManager.h>

phx::AsyncLoader::AsyncLoader(ThreadPoolHandle thread_pool_handle)
    : m_thread_pool_handle(thread_pool_handle)
{}

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

void phx::AsyncLoader::ThreadLoop()
{
    std::vector<LoadRequest> incoming;
    incoming.reserve(16); uint64_t loop_tick = 0;

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
            Result<AsyncResourceDescriptor> resource_descriptor = 
                Vfs::GetResourceDescriptorForAsync(req.virtual_path);

            if (resource_descriptor.HasError())
            {
                PHX_CORE_ERROR(
                    "AsyncLoader: Failed to load resource '{0}'. Unable to retrieve resource descriptor",
                    req.virtual_path.c_str());
                req.handle->state = ResourceState::Error;
                continue;
			}

			auto job = std::make_unique<ActiveJob>();
            job->ctx.handle = req.handle;
            job->ctx.resource_descriptor = resource_descriptor.GetValue();
            job->ctx.state_index = ResourceState::Loading;
            job->ctx.thread_pool_handle = m_thread_pool_handle;
            job->loader = req.loader_interface;

            req.handle->state = ResourceState::Loading;

            m_active_jobs.emplace_back(std::move(job));
        }

        incoming.clear();

        loop_tick++;
        bool did_work = false;
        for (size_t i = 0; i < m_active_jobs.size(); )
        {
            ActiveJob& job = *m_active_jobs[i];

            if (job.ctx.state_index == ResourceState::Waiting_dependencies)
            {
                // Don't check for dependencies every frame, as they can often take multiple frames to load and we don't want to waste CPU time checking on them.
                if ((loop_tick % 8) != 0)
                {
                    ++i;
                    continue;
                }
            }

            LoaderStepResult result = job.loader->Step(job.ctx);

            if (result == LoaderStepResult::Done || result == LoaderStepResult::WaitOnGpuTransition)
            {
                ResourceState final_state = (result == LoaderStepResult::Done) 
                    ? ResourceState::Loaded 
                    : ResourceState::Pending_gfx_transition;

                if (final_state == ResourceState::Pending_gfx_transition)
                    ResourceManager::PushToGpuTransitionQueue(job.ctx.handle);

                job.ctx.handle->state = ResourceState::Loading;

                if (i != m_active_jobs.size() - 1)
                    m_active_jobs[i] = std::move(m_active_jobs.back());

                m_active_jobs.pop_back();
                did_work = true;
            }
            else if (result == LoaderStepResult::Error)
            {
                job.ctx.handle->state = ResourceState::Error;

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

