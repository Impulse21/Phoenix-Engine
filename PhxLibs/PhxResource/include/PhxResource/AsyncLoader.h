#pragma once

#include <PhxResource/ResourceTypes.h>
#include <PhxResource/IResourceLoader.h>

#include <condition_variable>
#include <atomic>
#include <mutex>

namespace phx
{
    struct LoadRequest
    {
        RefCountPtr<Resource> handle;
        std::string virtual_path;
        IResourceLoader* loader_interface; // Pointer to the singleton loader (TextureLoader, etc.)
    };

    class AsyncLoader
    {
    public:
        AsyncLoader(ThreadPoolHandle thread_pool_handle);
        ~AsyncLoader();

        void Start();
        void Stop();

        void QueueRequest(const LoadRequest& req);

    private:
        void ThreadLoop();

        struct ActiveJob
        {
            LoadContext ctx;
            IResourceLoader* loader;
        };

    private:
        ThreadPoolHandle m_thread_pool_handle;
        std::mutex m_queue_mutex;
        std::vector<LoadRequest> m_pending_requests;

        std::condition_variable m_wake_signal;

        std::thread m_thread;
        std::atomic<bool> m_running = false;

        std::vector<std::unique_ptr<ActiveJob>> m_active_jobs;
    };
}
