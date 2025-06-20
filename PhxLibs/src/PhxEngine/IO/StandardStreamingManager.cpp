#include "PhxEngine/PhxEngine_pch.h"
#include "StandardStreamingManager.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/SystemTime.h>

using namespace phx;
using namespace phx::data;

namespace
{
struct StreamingRequestProcessor
{
	void operator()(StreamingRequest& request, StandardStreamingManager* streaming_manager)
	{
		PHX_CORE_INFO(
			"Processing StreamingRequest {0}:{1} with {2} operations.",
			request.request_id,
			request.debug_name,
			request.operations.size());

		CpuTimer processing_time;

		bool successful = true;

		// Begin Upload Context
		for (auto& operation : request.operations)
		{
			ErrorCode error_code = ProcessOperation(streaming_manager, operation);
			successful &= error_code == ErrorCode::Success;
		}

		// Submit Upload context if possible
		
		// Begin Upload Context

		StreamingResult result = { 
			.request_id = request.request_id,
			.error_code = successful ? ErrorCode::Success : ErrorCode::Unknown };


		JobSystem::SubmitJob(
			[cb = std::move(request.on_complete), res = std::move(result)](JobContext const&) mutable
			{
				cb(res);
			},
			JobSystem::Priority::Low);
		
		CpuTimeStep elapsed_time = processing_time.Elapsed();
		PHX_CORE_INFO(
			"Processing StreamingRequest {0}:{1} with {2} operations. Took {3} (ms)",
			request.request_id,
			request.debug_name,
			request.operations.size(),
			elapsed_time.GetMilliseconds());
	}

	ErrorCode ProcessOperation(StandardStreamingManager* streaming_manager, StreamingOperation& operation)
	{
		ErrorCode ret_val = ErrorCode::Success;
		std::visit([&](auto&& active_source_data) {
			ret_val = ProcessSource(streaming_manager, active_source_data, operation.source, operation.destination);
		}, operation.source.data);

		return ret_val;
	}
	template<typename TSource>
	ErrorCode ProcessSource(StandardStreamingManager* streaming_manager, TSource& source_data_active_type, StreamingSource& source_info, StreamingDestination& destination_info)
	{
		if constexpr (std::is_same_v<std::decay_t<TSource>, AsyncResourceDescriptor>)
		{
			if (!ProcessAsyncResourceDesc(source_data_active_type, streaming_manager, source_info))
				return ErrorCode::Unknown;
		}
		else if constexpr (std::is_same_v<std::decay_t<TSource>, ReadableCpuMemoryBuffer>)
		{
			ReadableCpuMemoryBuffer& buffer = source_data_active_type;
			data_to_transfer = buffer.get() + source_info.offset;
			effective_transfer_size = source_info.size;
		}
		else
		{
			PHX_CORE_ERROR("Unknown Source Type!");
			return ErrorCode::Unknown;
		}

		ErrorCode ret_val = ErrorCode::Success;
		// Process Destination
		std::visit([&](auto&& dest_active_type) {
			if constexpr (std::is_same_v<std::decay_t<decltype(dest_active_type)>, WriteableCpuMemoryBuffer>)
			{
				// Case A: Destination is CPU Memory Buffer (WritableCpuMemoryBuffer)
				WriteableCpuMemoryBuffer& dest_buffer = dest_active_type;
				if (!dest_buffer || dest_buffer.get() == nullptr)
				{
					ret_val = ErrorCode::InvalidDestination;
					return;
				}

				if (effective_transfer_size > destination_info.size)
				{
					ret_val = ErrorCode::InvalidDestination;
					return;
				}

				std::memcpy(dest_buffer.get() + destination_info.offset, data_to_transfer, effective_transfer_size);
				ret_val = ErrorCode::Success;
			}
			else if constexpr (std::is_same_v<std::decay_t<decltype(dest_active_type)>, GpuResourceDestinationInfo>)
			{
				GpuResourceDestinationInfo& gpu_dest_info = dest_active_type;
				std::visit([&](auto&& handle_type) {
					if constexpr (std::is_same_v<std::decay_t<decltype(handle_type)>, rhi::TextureHandle>)
					{
						// TODO Texture uploading
					}
					else if constexpr (std::is_same_v<std::decay_t<decltype(handle_type)>, rhi::GpuBufferHandle>)
					{
						// TODO Buffer Uploading
					}
					else
					{
						ret_val = ErrorCode::Unknown;
						return;
					}

					ret_val = ErrorCode::Success;
					},
					gpu_dest_info.handle);
			}
			else
			{
				ret_val = ErrorCode::Unknown;
			}
			},
			destination_info.target);

		return ret_val;
	}

	bool ProcessAsyncResourceDesc(AsyncResourceDescriptor& descriptor, StandardStreamingManager* streaming_manager, StreamingSource& source_info)
	{
		// Determine effective source data pointer and size
		if (descriptor.type == AsyncDataSourceType::Embedded)
		{
			data_to_transfer = descriptor.memory_buffer_ptr + source_info.offset;
			effective_transfer_size = source_info.size;
		}
		else
		{
			platform::PlatformFileHandle file_handle = streaming_manager->FindOrCreateHandle(descriptor.os_path_or_pak_path);

			if (!file_handle.IsValid())
			{
				PHX_CORE_ERROR("Failed to open OS File: {0}", descriptor.os_path_or_pak_path);
				return false;
			}

			const int64_t final_seek_offset =
				static_cast<int64_t>(descriptor.offset_in_pak) +
				static_cast<int64_t>(source_info.offset);

			if (!Platform::Get().SeekFile(file_handle, final_seek_offset, platform::FileSeekOrigin::Begin))
			{
				PHX_CORE_ERROR("Failed to seek in file: {0}", descriptor.os_path_or_pak_path);
				return false;
			}

			intermediate_buffer = std::make_shared<char[]>(source_info.size);
			effective_transfer_size = source_info.size;
			data_to_transfer = intermediate_buffer.get();
			size_t bytes_read = Platform::Get().ReadFile(file_handle, intermediate_buffer.get(), source_info.size);

			if (bytes_read != source_info.size)
			{
				PHX_CORE_ERROR(
					"Short read from file {0}. Expected {1}, got {2}",
					descriptor.os_path_or_pak_path,
					source_info.size,
					bytes_read);
				return false;
			}
		}
		return true;
	}

	std::shared_ptr<char[]> intermediate_buffer = nullptr;
	const char* data_to_transfer = nullptr;
	uint64_t effective_transfer_size = 0;

};
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
}

void phx::StandardStreamingManager::Submit(StreamingRequest&& request)
{
	{
		std::scoped_lock lock(m_queueMutex);
		request.request_id = data::RequestIdGenerator();
		m_requestQueue.push_back(std::move(request));
	}

	m_cv.notify_one(); // Signal the streaming thread that new work is available
}

void phx::StandardStreamingManager::Tick(float /*delta_time*/)
{
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
		data::StreamingRequest currentRequest;
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