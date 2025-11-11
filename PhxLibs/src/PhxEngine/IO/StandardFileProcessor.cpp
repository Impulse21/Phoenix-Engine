#include "PhxEngine/PhxEngine_pch.h"
#include "StandardFileProcessor.h"

#include <PhxRhi/ISubmissionManager.h>
#include <PhxRhi/ICommandBuffer.h>

#include <PhxEngine/JobSystem.h>
using namespace phx;

void StandardFileProcessor::ProcessRequest(StreamingRequest&& request)
{
	StreamingResult result = {
		.request_id = request.request_id,
		.status_array = {0}
	};

	bool has_gpu_work = false;
	rhi::CommandBufferHandle ctx_handle = rhi::NULL_CMD_HANDLE;
	for (size_t i = 0; i < request.operations.size(); i++)
	{
		// Pass the WIP command buffer and state to ProcessOperation
		bool gpu_operation = false;

		StreamingOperation& operation = request.operations[i];
		ErrorCode error_code = ProcessStreamingTransfer(operation.source, operation.destination, gpu_operation);

		has_gpu_work |= gpu_operation;

		if (error_code != ErrorCode::Success)
			result.status_array.set(i);
	}

	if (has_gpu_work)
	{
		std::scoped_lock lock(m_batch_mutex);

		ctx_handle = m_wip_cmd_list;

		PendingCallback callback_entry;
		callback_entry.on_complete = std::move(request.on_complete);
		callback_entry.result = std::move(result);

		m_wip_callbacks.push_back(std::move(callback_entry));
	}
	else
	{
		JobSystem::SubmitJob(
			[cb = std::move(request.on_complete), res = std::move(result)](JobContext const&) mutable
			{
				cb(res);
			},
			JobSystem::Priority::Low);
	}
}

void phx::StandardFileProcessor::SubmitBatchedWork(IAllocator* frame_allocator, rhi::ISubmissionManager* submission_manager)
{
	PHX_PROFILE;

	// Frame allocator would be best to use here.
	SpanMutable<GpuWorkItem> batches_to_submit;
	{
		std::scoped_lock _(m_batch_mutex);
		batches_to_submit = AllocateArray<GpuWorkItem>(frame_allocator, m_pending_batch.size());
		
		for (size_t i = 0; i < m_pending_batch.size(); i++)
		{
			batches_to_submit[i] = std::move(m_pending_batch[i]);
		}
		m_pending_batch.clear();
	}

	// Request a free Gpu Pending work item
	// TODO: Request a pending work item from the pool.

	size_t pending_work_index = 0;
	if (!m_free_indices.empty())
	{
		pending_work_index = m_free_indices.back();
		m_free_indices.pop_back();
	}
	else
	{
		pending_work_index = m_work_slots.size();
		m_work_slots.emplace_back();
	}

	GpuPendingWork& pending_work = m_work_slots[pending_work_index];

	SpanMutable<rhi::ICommandBuffer*> commands_to_submit = AllocateArray<rhi::ICommandBuffer*>(frame_allocator, m_pending_batch.size());
	for (size_t i = 0; i < batches_to_submit.Size(); ++i)
	{
		// 1. Move the callback
		pending_work.callbacks.emplace_back(std::move(batches_to_submit[i].on_complete));

		// 2. (THE FIX) Get the command list handle
		commands_to_submit[i] = batches_to_submit[i].command_buffer;
	}
	
	pending_work.fence = submission_manager->Submit(rhi::CommandQueueType::Copy, commands_to_submit, {});
	m_inflight_indices.push_back(pending_work_index);
}

void phx::StandardFileProcessor::PullCompletions(rhi::ISubmissionManager* submission_manager)
{
	for (size_t inflight_index : m_inflight_indices)
	{
		GpuPendingWork& inflight_work = m_work_slots[inflight_index];

		if (!submission_manager->IsFenceCompleted(inflight_work.fence))
			continue;

		for (auto& callback : inflight_work.callbacks)
		{
			JobSystem::SubmitJob(JobSystem::Priority::Low, [&]() {
				callback();
			});
		}

		// retire work item
		inflight_work.callbacks.clear();
		m_free_indices.push_back(inflight_index);
	}
}


platform::PlatformFileHandle phx::StandardFileProcessor::FindOrCreateHandle(std::string const& file_path)
{
	// TODO: CLean up cache
	std::scoped_lock _(m_file_handle_cache_mutex);

	auto it = m_file_handle_cache.find(file_path);
	if (it != m_file_handle_cache.end())
	{
		return it->second; // Use cached handle
	}

	// File not in cache, open it using the platform layer.
	platform::PlatformFileHandle file_handle = Platform::Get().OpenFile(file_path, "rb").GetValue();
	if (file_handle.IsValid())
	{
		m_file_handle_cache[file_path] = file_handle;
	}

	return file_handle;
}

ErrorCode phx::StandardFileProcessor::ProcessStreamingTransfer(
	StreamingSource& source_info,
	StreamingDestination& destination_info,
	bool& gpu_operation)
{
	// TODO: Check for overrun.
	const std::byte* src_ptr = nullptr;
	std::byte* dest_ptr = nullptr;

	std::visit([&](auto&& target) {
		using TTarget = std::decay_t<decltype(target)>;
		if constexpr (std::is_same_v<TTarget, CpuResourceDestinationInfo>)
		{
			CpuResourceDestinationInfo& cpu_dest_info = target;
			dest_ptr = static_cast<std::byte*>(cpu_dest_info.handle) + destination_info.offset;
		}
		else if constexpr (std::is_same_v<TTarget, GpuResourceDestinationInfo>)
		{
			gpu_operation = true;
			// TODO: Get mapped memory from command list.
		}
	}, destination_info.target);

	ErrorCode ret_val = ErrorCode::Success;
	std::visit([&](auto&& src_data)
		{
			using TSource = std::decay_t<decltype(src_data)>;
			if constexpr (std::is_same_v<TSource, AsyncResourceDescriptor>)
			{
				if (!ProcessAsyncResourceDesc(src_data, source_info, dest_ptr))
				{
					ret_val = ErrorCode::Unknown;
					return;
				}
			}
			else if constexpr (std::is_same_v<TSource, ReadableCpuMemoryBuffer>)
			{
				ReadableCpuMemoryBuffer& buffer = src_data;

				if (!buffer)
				{
					PHX_CORE_ERROR("Source ReadableCpuMemoryBuffer is null!");
					ret_val = ErrorCode::InvalidSource;
					return;
				}

				src_ptr = buffer + source_info.offset;
				std::memcpy(dest_ptr, src_ptr, source_info.size);
			}
		},
		source_info.data); // We visit the source variant here

	return ErrorCode::Success;
}

bool phx::StandardFileProcessor::ProcessAsyncResourceDesc(
	AsyncResourceDescriptor& descriptor,
	StreamingSource& source_info,
	std::byte* dest_ptr)
{
	const std::byte* src_ptr = nullptr;
	if (descriptor.type == AsyncDataSourceType::Embedded)
	{
		src_ptr = reinterpret_cast<const std::byte*>(descriptor.memory_buffer_ptr) + source_info.offset;
		std::memcpy(dest_ptr, src_ptr, source_info.size);
		return true;
	}

	platform::PlatformFileHandle file_handle = FindOrCreateHandle(descriptor.os_path_or_pak_path);

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

	size_t bytes_read = Platform::Get().ReadFile(file_handle, dest_ptr, source_info.size);

	if (bytes_read != source_info.size)
	{
		PHX_CORE_ERROR(
			"Short read from file {0}. Expected {1}, got {2}",
			descriptor.os_path_or_pak_path,
			source_info.size,
			bytes_read);
		return false;
	}

	return true;

}
