#include "PhxEngine/PhxEngine_pch.h"
#include "StandardStreamingManager.h"
#include <mutex>

#include <PhxEngine/JobSystem.h>
#include <PhxCore/IO/MemoryRegion.h>

using namespace phx;
using namespace phx::data;

struct StreamingRequestProcessor
{
    void operator()(StreamingRequest& request, StandardStreamingManager* streaming_manager) 
    {
        PHX_CORE_INFO(
            "Processing StreamingRequest {0}:{1}\n\tSource offset: {3}, size: {4}\n\Destination offset: {3}, size: {4}",
            request.request_id, request.debug_name,
            request.source.offset, request.source.size,
            request.destination.offset, request.destination.size);

        std::visit([&](auto&& active_source_data) {
            ProcessSource(streaming_manager, active_source_data, request.source, request.destination, request);
            }, request.source.data);
    }

    template<typename TSource>
    void ProcessSource(StandardStreamingManager* streaming_manager, TSource& source_data_active_type, StreamingSource& source_info, StreamingDestination& destination_info, StreamingRequest& request)
    {
        if constexpr (std::is_same_v<std::decay_t<TSource>, AsyncResourceDescriptor>)
        {
            if (!ProcessAsyncResourceDesc(source_data_active_type, streaming_manager, source_info))
            {
                request.on_complete({ request.request_id, ErrorCode::Unknown });
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<TSource>, CpuMemoryBuffer>)
        {
            CpuMemoryBuffer& buffer = source_data_active_type;
            data_to_transfer = buffer.get() + source_info.offset;
            effective_transfer_size = source_info.size;
        }
        else
        {
            PHX_CORE_ERROR("Unknown Source Type!");
            // request.on_complete({ request.request_id, ErrorCode::Unknown });
            return;
        }
        
        // Process Destination
        std::visit([&](auto&& dest_active_type) {
            if constexpr (std::is_same_v<std::decay_t<decltype(dest_active_type)>, CpuMemoryBuffer>)
            {
                // Case A: Destination is CPU Memory Buffer (WritableCpuMemoryBuffer)
                WritableCpuMemoryBuffer& dest_buffer = dest_active_type;
                std::cout << "      Destination is CPU Memory Buffer. Ptr: " << static_cast<void*>(dest_buffer.get()) << ", Expected Size: " << destination_info.size << std::endl;
                if (!dest_buffer || dest_buffer.get() == nullptr) {
                    std::cout << "        ERROR: CPU Destination Buffer is null!" << std::endl;
                    request.on_complete({ request.request_id, ErrorCode::InvalidDestination });
                    return;
                }
                if (effective_transfer_size > destination_info.size) { // Check for buffer overflow
                    std::cout << "        ERROR: Source data too large for destination buffer!" << std::endl;
                    request.on_complete({ request.request_id, ErrorCode::InvalidDestination });
                    return;
                }
                std::memcpy(dest_buffer.get() + destination_info.offset, data_to_transfer, effective_transfer_size);
                std::cout << "        Data copied to CPU memory destination." << std::endl;
                request.on_complete({ request.request_id, ErrorCode::Success });
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(dest_active_type)>, GpuResourceDestinationInfo>)
            {
                // Case B: Destination is GPU Resource
                GpuResourceDestinationInfo& gpu_dest_info = dest_active_type;
                std::cout << "      Destination is GPU Resource." << std::endl;
                std::visit([&](auto&& handle_type) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(handle_type)>, rhi::TextureHandle>) {
                        std::cout << "        Specific GPU target: Texture (ID: " << handle_type.id << ")" << std::endl;
                        // Call RHI upload function for texture
                        // m_rhiDevice->UploadTexture(handle_type, data_to_transfer, effective_transfer_size, gpu_dest_info.offset, gpu_dest_info.size);
                        std::cout << "        Simulating RHI Texture Upload." << std::endl;
                    }
                    else if constexpr (std::is_same_v<std::decay_t<decltype(handle_type)>, rhi::GpuBufferHandle>) {
                        std::cout << "        Specific GPU target: Buffer (ID: " << handle_type.id << ")" << std::endl;
                        // Call RHI upload function for buffer
                        // m_rhiDevice->UploadBuffer(handle_type, data_to_transfer, effective_transfer_size, gpu_dest_info.offset, gpu_dest_info.size);
                        std::cout << "        Simulating RHI Buffer Upload." << std::endl;
                    }
                    else {
                        std::cout << "        ERROR: Unknown GPU Resource Handle Type!" << std::endl;
                        request.on_complete({ request.request_id, ErrorCode::InvalidDestination });
                        return;
                    }
                    request.on_complete({ request.request_id, ErrorCode::Success }); // Simulate success after RHI call
                    }, gpu_dest_info.handle);
            }
            else
            {
                std::cout << "      ERROR: Unknown Destination Type!" << std::endl;
                request.on_complete({ request.request_id, ErrorCode::Unknown });
            }
            }, destination_info.target);
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

void phx::StandardStreamingManager::SubmitBatch(SpanMutable<data::StreamingRequest> requests)
{
    {
        std::scoped_lock lock(m_queueMutex);
        for (auto& request : requests)
        {
            request.request_id = data::RequestIdGenerator();
            m_requestQueue.push_back(std::move(request));
        }
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

#if false
void phx::StandardStreamingManager::ProcessRequest(data::StreamingRequest& /*request*/)
{
    data::StreamingRequest result;
    result.user_context = request.user_context;

    if (request.resource_descriptor.type == data::AsyncDataSourceType::MemoryBuffer)
    {
        const auto& desc = request.resource_descriptor;
        void* dest_buffer = request.cpu_destination_buffer;

        PHX_ASSERT(dest_buffer);
        memcpy(
            dest_buffer,
            desc.memory_buffer_ptr + request.read_offset_within_resource,
            request.bytes_to_read);

        result.bytes_actually_read = request.bytes_to_read;
        result.success = true;
    }
    else
    {
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
    }

    if (!request.callback)
        return;

    // Not sure I want this to be put back on the job system.
    JobSystem::SubmitJob(
        [cb = std::move(request.on_), res = std::move(result)](JobContext const&) mutable 
        {
            cb(res);
        },
        JobSystem::Priority::Low);
}
#endif
