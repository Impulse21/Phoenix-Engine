#include "PhxEngine/PhxEngine_pch.h"
#include "StandardStreamingManager.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/SystemTime.h>

#include <PhxRhi/PhxRhi.h>

using namespace phx;

namespace
{

struct StreamingRequestProcessor
{
	static std::pair<std::byte*, ErrorCode> GetCpuDestinationPointer(
		const std::variant<std::shared_ptr<char[]>, void*>& handle_variant,
		uint64_t dest_offset_bytes,
		uint64_t dest_total_bytes,
		uint64_t transfer_size_bytes)
	{
		std::byte* dest_ptr = nullptr;
		ErrorCode error = ErrorCode::Success;

		// Check for buffer overflow *before* visiting.
		// This logic was flawed in the original; it's now corrected to
		// check (offset + size) against the total buffer size.
		if ((dest_offset_bytes + transfer_size_bytes) > dest_total_bytes)
		{
			return { nullptr, ErrorCode::InvalidDestination }; // Buffer overflow
		}

		std::visit([&](auto&& handle)
			{
				using THandle = std::decay_t<decltype(handle)>;

				if constexpr (std::is_same_v<THandle, std::shared_ptr<char[]>>)
				{
					if (!handle) 
					{
						error = ErrorCode::InvalidDestination;
						return;
					}
					dest_ptr = reinterpret_cast<std::byte*>(handle.get()) + dest_offset_bytes;
				}
				else if constexpr (std::is_same_v<THandle, void*>)
				{
					if (!handle) 
					{
						error = ErrorCode::InvalidDestination;
						return;
					}
					dest_ptr = static_cast<std::byte*>(handle) + dest_offset_bytes;
				}
				else
				{
					error = ErrorCode::Unknown; // Should not happen if variant is correct
				}
			}, handle_variant);

		return { dest_ptr, error };
	}

	void operator()(StreamingRequest& request, StandardStreamingManager* streaming_manager)
	{
		PHX_CORE_INFO(
			"Processing StreamingRequest {0}:{1} with {2} operations.",
			request.request_id,
			request.debug_name,
			request.operations.size());

		CpuTimer processing_time;

		StreamingResult result = {
			.request_id = request.request_id,
			.status_array = {0}
		};

		RHI::CommandBufferHandle ctx_handle = RHI::BeginAsyncCommandBuffer(RHI::CommandQueueType::Copy);
		for (size_t i = 0; i < request.operations.size(); i++)
		{
			ErrorCode error_code = ProcessOperation(streaming_manager, ctx_handle, request.operations[i]);
			if (error_code != ErrorCode::Success)
				result.status_array.set(i);
		}

		if (result.status_array.none())
		{
			result.error_code = ErrorCode::Success;
		}

		RHI::SubmitAsyncCommandBuffer({ ctx_handle });

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

	ErrorCode ProcessOperation(StandardStreamingManager* streaming_manager, RHI::CommandBufferHandle ctx_handle, StreamingOperation& operation)
	{
		ErrorCode ret_val = ErrorCode::Success;
		std::visit([&](auto&& active_source_data) {
			ret_val = ProcessStreamingTransfer(streaming_manager, ctx_handle, active_source_data, operation.source, operation.destination);
		}, operation.source.data);

		return ret_val;
	}

	template<typename TSource>
	ErrorCode ProcessStreamingTransfer(
		StandardStreamingManager* streaming_manager,
		RHI::CommandBufferHandle /*unused_ctx_handle*/,
		TSource& source_data,
		StreamingSource& source_info,
		StreamingDestination& destination_info)
	{
		// --- 1. Declare Transfer Variables ---
		// Moved from the bottom of the function to the top.
		// Using const std::byte* is safer for raw data pointers.
		if constexpr (std::is_same_v<std::decay_t<TSource>, AsyncResourceDescriptor>)
		{
			if (!ProcessAsyncResourceDesc(source_data, streaming_manager, source_info))
			{
				return ErrorCode::Unknown;
			}
		}
		else if constexpr (std::is_same_v<std::decay_t<TSource>, ReadableCpuMemoryBuffer>)
		{
			ReadableCpuMemoryBuffer& buffer = source_data;
			if (!buffer || !buffer.get())
			{
				PHX_CORE_ERROR("Source ReadableCpuMemoryBuffer is null!");
				return ErrorCode::InvalidSource;
			}

			data_to_transfer = buffer.get() + source_info.offset;
			effective_transfer_size = source_info.size;
		}
		else
		{
			PHX_CORE_ERROR("Unknown Source Type!");
			return ErrorCode::Unknown;
		}

		ErrorCode ret_val = ErrorCode::Unknown; // Default to error
		std::visit([&](auto&& dest_active_type)
			{
				using TDest = std::decay_t<decltype(dest_active_type)>;

				if constexpr (std::is_same_v<TDest, CpuResourceDestinationInfo>)
				{
					// --- Destination is CPU Memory ---
					CpuResourceDestinationInfo& cpu_dest_info = dest_active_type;

					// Use our clean helper function
					auto [dest_ptr, error] = GetCpuDestinationPointer(
						cpu_dest_info.handle,
						destination_info.offset,
						destination_info.size,
						effective_transfer_size
					);

					if (error != ErrorCode::Success)
					{
						ret_val = error;
						return;
					}

					std::memcpy(dest_ptr, data_to_transfer, effective_transfer_size);
					ret_val = ErrorCode::Success;

					// FIX: Removed the dead/erroneous code block that was
					// here in the original (the WriteableCpuMemoryBuffer block).
				}
				else if constexpr (std::is_same_v<TDest, GpuResourceDestinationInfo>)
				{
					// --- Destination is GPU Memory (CPU-to-GPU Upload) ---
					GpuResourceDestinationInfo& gpu_dest_info = dest_active_type;

					// This is where you would queue the upload using your RHI
					// or streaming manager.
					std::visit([&](auto&& handle_type) {
						using THandle = std::decay_t<decltype(handle_type)>;

						if constexpr (std::is_same_v<THandle, RHI::TextureHandle>)
						{
							// TODO: Texture uploading logic
							// e.g., streaming_manager->QueueTextureUpload(handle_type, ...);
							ret_val = ErrorCode::Success; // Placeholder
						}
						else if constexpr (std::is_same_v<THandle, RHI::GpuBufferHandle>)
						{
							// TODO: Buffer Uploading logic
							// e.g., streaming_manager->QueueBufferUpload(handle_type, ...);
							ret_val = ErrorCode::Success; // Placeholder
						}
						else
						{
							ret_val = ErrorCode::Unknown;
						}
						}, gpu_dest_info.handle);
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
		request.request_id = RequestIdGenerator();
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