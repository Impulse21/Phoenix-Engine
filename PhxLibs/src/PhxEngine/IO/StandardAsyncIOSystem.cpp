#include "PhxEngine/PhxEngine_pch.h"
#include "StandardAsyncIOSystem.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>

void phx::StandardAsyncIOSystem::Initialize()
{
    m_shutdown = false;
	(void)m_vfs;
    JobSystem::SubmitJobToStreaming([this](JobContext const&) {
        this->StreamingThreadLoop();
    }); // Target your dedicated streaming thread)
}

void phx::StandardAsyncIOSystem::Shutdown()
{
    {
        std::scoped_lock lock(m_queueMutex);
        m_shutdown = true;
    }

    m_cv.notify_one(); // Wake up the streaming thread to exit

    JobSystem::Wait(JobSystem::Type::Streaming);

    std::scoped_lock _(m_fileHandleCacheMutex);

    for (auto& pair : m_fileHandleCache)
    {
        Platform::Get().CloseFile(pair.second);
    }

    m_fileHandleCache.clear();
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

void phx::StandardAsyncIOSystem::ProcessReadRequest(data::AsyncReadRequest& request)
{
    data::AsyncReadResult result;
    result.user_context = request.user_context;

    platform::PlatformFileHandle file_handle;

    {
        std::scoped_lock _(m_fileHandleCacheMutex);

        auto it = m_fileHandleCache.find(request.resource_descriptor.os_path_or_pak_path);
        if (it != m_fileHandleCache.end()) 
        {
            file_handle = it->second; // Use cached handle
        }
        else 
        {
            // File not in cache, open it using the platform layer.
            file_handle = Platform::Get().OpenFile(request.resource_descriptor.os_path_or_pak_path, "rb").GetValue();
            if (file_handle.IsValid()) 
            {
                m_fileHandleCache[request.resource_descriptor.os_path_or_pak_path] = file_handle;
            }
        }
    }

    if (!file_handle.IsValid())
    {
        result.success = false;
        result.error_message = "Failed to open OS File: " + request.resource_descriptor.os_path_or_pak_path;
    }
    else
    {
        const int64_t final_seek_offset = 
            static_cast<int64_t>(request.resource_descriptor.offset_in_pak) +
            static_cast<int64_t>(request.read_offset_within_resource);

        // 3. Seek to the calculated offset using the platform layer.
        if (!Platform::Get().SeekFile(file_handle, final_seek_offset, platform::FileSeekOrigin::Begin))
        {
            result.success = false;
            result.error_message = "Failed to seek in file.";
        }
        else
        {
            result.data_buffer.resize(request.bytes_to_read);
            size_t bytes_read = Platform::Get().ReadFile(file_handle, result.data_buffer.data(), request.bytes_to_read);

            result.bytes_actually_read = bytes_read;

            if (bytes_read == request.bytes_to_read)
            {
                result.success = true;
            }
            else
            {
                result.success = false;
                result.error_message = 
                    "Short read from file. Expected " + std::to_string(request.bytes_to_read) +
                    ", got " + std::to_string(bytes_read) + ".";
                result.data_buffer.resize(bytes_read);
            }
        }
    }

    if (!request.callback)
        return;

    // Not sure I want this to be put back on the job system.
    JobSystem::SubmitJob(
        [cb = std::move(request.callback), res = std::move(result)](JobContext const&) mutable 
        {
            cb(res);
        },
        JobSystem::Priority::Low);
}
