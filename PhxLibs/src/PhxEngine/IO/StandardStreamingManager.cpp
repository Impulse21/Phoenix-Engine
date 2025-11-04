#include "PhxEngine/PhxEngine_pch.h"
#include "StandardStreamingManager.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/SystemTime.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;

// This struct replaces the member variables from the old StreamingRequestProcessor
// We create one of these *per request* inside the loop.
struct phx::StandardStreamingManager::ProcessingState
{
	MemoryBuffer intermediate_buffer;
	const std::byte* data_to_transfer = nullptr;
	uint64_t effective_transfer_size = 0;
};

namespace
{
	// This helper function remains the same as your original
	std::pair<std::byte*, ErrorCode> GetCpuDestinationPointer(
		const std::variant<std::shared_ptr<char[]>, void*>& handle_variant,
		uint64_t dest_offset_bytes,
		uint64_t dest_total_bytes,
		uint64_t transfer_size_bytes)
	{
		// ... (Same logic as your .cpp file) ...
	}
}
void phx::StandardStreamingManager::Initialize()
{
	m_shutdown = false;
	(void)m_vfs;
	JobSystem::SubmitJobToStreaming([this](JobContext const&) {
		this->StreamingThreadLoop();
	}); // Target your dedicated streaming thread)
}

void phx::StandardStreamingManager::Shutdown()
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
	// NOTE: Shutdown() should have already been called, 
	// but this is good practice for safety.
	if (!m_shutdown)
	{
		Shutdown();
	}

	// Clean up the object pool
	for (auto* work : m_pending_gpu_work)
	{
		delete work;
	}

	m_pending_gpu_work.clear();

	for (auto* work : m_free_gpu_work_pool)
	{
		delete work;
	}
	m_free_gpu_work_pool.clear();
}

void phx::StandardStreamingManager::Submit(StreamingRequest&& request)
{
	{
		std::scoped_lock lock(m_queueMutex);
		request.request_id = RequestIdGenerator();
		m_requestQueue.push_back(std::move(request));
	}

	m_cv.notify_one(); // Signal the streaming thread that new work is available
}

void phx::StandardStreamingManager::SubmitStreamingCopies()
{
	RHI::CommandBufferHandle cmd_buffer_to_submit;
	std::vector<PendingCallback> callbacks_to_submit;

	// 1. Lock and steal the work-in-progress batch
	{
		std::unique_lock lock(m_batch_mutex);
		if (!m_wip_cmd_list.IsValid())
		{
			return; // Nothing to submit
		}

		cmd_buffer_to_submit = m_wip_cmd_list;
		callbacksToSubmit = std::move(m_wipCallbacks);

		// Reset the WIP members so the streaming thread can start a new batch
		m_wipCmdBuffer = RHI_NULL_HANDLE;
		m_wipCallbacks.clear();
	} // Unlock m_batchMutex

	// 2. Submit the *entire batch* to the GPU (outside the lock)
	rhi::FenceHandle fence = RHI::SubmitAsyncCommandBuffer({ cmdBufferToSubmit });

	// 3. Add this *one* fence with its *list* of callbacks
	{
		std::lock_guard lock(m_pendingGpuWorkMutex);
		m_pendingGpuWork.push_back({
			.fence = fence,
			.callbacks = std::move(callbacksToSubmit)
			});
	}
}

platform::PlatformFileHandle phx::StandardStreamingManager::FindOrCreateHandle(std::string const& file_path)
{
	std::scoped_lock _(m_fileHandleCacheMutex);

	auto it = m_fileHandleCache.find(file_path);
	if (it != m_fileHandleCache.end())
	{
		return it->second; // Use cached handle
	}

	// File not in cache, open it using the platform layer.
	platform::PlatformFileHandle file_handle = Platform::Get().OpenFile(file_path, "rb").GetValue();
	if (file_handle.IsValid())
	{
		m_fileHandleCache[file_path] = file_handle;
	}

	return file_handle;
}

void phx::StandardStreamingManager::StreamingThreadLoop()
{
	while (true)
	{
		StreamingRequest currentRequest;
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

		StreamingRequestProcessor processor;
		processor(currentRequest, this);
	}

	PHX_CORE_INFO("AsyncIOManager: Streaming thread shutting down.");
}