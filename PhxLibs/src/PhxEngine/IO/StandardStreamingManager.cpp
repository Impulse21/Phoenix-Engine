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

	{
		std::unique_lock lock(m_batch_mutex);
		if (!m_wip_cmd_list.IsValid())
		{
			return; // Nothing to submit
		}

		cmd_buffer_to_submit = m_wip_cmd_list;
		callbacks_to_submit = std::move(m_wip_callbacks);

		// Reset the WIP members so the streaming thread can start a new batch
		m_wip_cmd_list = RHI::INVALID_COMMAND_HANDLE;
		m_wip_callbacks.clear();
	}

	RHI::FenceHandle fence = RHI::SubmitAsyncCommandBuffer({ cmd_buffer_to_submit });

	GpuPendingWork* work = nullptr;
	{
		std::lock_guard lock(m_gpu_work_mutex);
		if (!m_free_gpu_work_pool.empty())
		{
			// 1. Get from pool
			work = m_free_gpu_work_pool.front();
			m_free_gpu_work_pool.pop_front();
		}
	}

	if (work == nullptr)
		work = new GpuPendingWork();

	// 3. Fill the recycled object
	work->fence = fence;
	work->callbacks = std::move(callbacks_to_submit); // Move the callbacks in

	// 3. Add this *one* fence with its *list* of callbacks
	{
		std::lock_guard lock(m_gpu_work_mutex);
		m_pending_gpu_work.push_back(work);
	}
}

void phx::StandardStreamingManager::PollGpuCompletions()
{
	std::lock_guard lock(m_gpu_work_mutex);

	for (int i = m_pending_gpu_work.size() - 1; i >= 0; --i)
	{
		auto* work = m_pending_gpu_work[i]; // It's a pointer

		if (true)// RHI::IsFenceSignaled(work->fence))
		{
			// ... (Same logic to submit jobs for each callback) ...
			for (auto& cb : work->callbacks)
			{
				JobSystem::SubmitJob([cb]() {
					cb.on_complete(cb.result);
				}, JobSystem::Priority::Low, nullptr);
			}

			RHI::DestroyFence(work->fence);
			work->callbacks.clear()

			m_freeGpuWorkPool.push_back(work); // 1. Return object to pool

			// 2. Remove from pending list
			std::swap(m_pending_gpu_work[i], m_pending_gpu_work.back());
			m_pending_gpu_work.pop_back();
		}
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

		{
			std::lock_guard lock(m_batch_mutex);

			// 1. Get a command buffer if we don't have one
			if (!m_wip_cmd_list.IsValid()) 
			{
				m_wip_cmd_list = RHI::BeginAsyncCommandBuffer(RHI::CommandQueueType::Copy);
			}

			PHX_CORE_INFO("Processing StreamingRequest {0}...", currentRequest.debug_name);

			// 2. Process all operations
			StreamingResult result = {
				.request_id = currentRequest.request_id,
				.status_array = {0}
			};

			// Create a state object *for this request*
			ProcessingState state;

			for (size_t i = 0; i < currentRequest.operations.size(); i++)
			{
				// Pass the WIP command buffer and state to ProcessOperation
				ErrorCode error_code = ProcessOperation(m_wip_cmd_list, state, currentRequest.operations[i]);
				if (error_code != ErrorCode::Success) 
				{
					result.status_array.set(i);
				}
			}

			if (result.status_array.none()) 
			{
				result.error_code = ErrorCode::Success;
			}

			// 3. Add this request's callback to the batch
			//    (We DO NOT submit the job here)
			m_wip_callbacks.push_back({
				.on_complete = std::move(currentRequest.on_complete),
				.result = std::move(result)
				});
		} // Release m_batchMutex
	}

	PHX_CORE_INFO("AsyncIOManager: Streaming thread shutting down.");
}