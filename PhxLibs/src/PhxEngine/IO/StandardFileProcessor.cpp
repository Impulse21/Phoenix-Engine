#include "PhxEngine/PhxEngine_pch.h"
#include "StandardFileProcessor.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxRhi/PhxRhi_Utils.h>

#include <PhxEngine/JobSystem.h>

using namespace phx;

phx::StandardFileProcessor::~StandardFileProcessor()
{
	for (auto& [_, file_handle] : m_file_handle_cache)
		Platform::Get().CloseFile(file_handle);
}

void StandardFileProcessor::ProcessRequest(StreamingRequest&& request)
{
	StreamingResult result = {
		.request_id = request.request_id,
		.status_array = {0},
		.error_code = ErrorCode::Success,
	};

	bool has_gpu_work = false;
	rhi::CmdHandle cmd_buffer = ~0u;
	for (size_t i = 0; i < request.operations.size(); i++)
	{
		// Pass the WIP command buffer and state to ProcessOperation
		bool gpu_operation = false;

		StreamingOperation& operation = request.operations[i];
		ErrorCode error_code = ProcessStreamingTransfer(
			operation.source,
			operation.destination,
			gpu_operation,
			cmd_buffer);

		has_gpu_work |= gpu_operation;

		result.error_code = error_code;
		if (error_code != ErrorCode::Success)
			result.status_array.set(i);
	}

	if (has_gpu_work)
	{
		std::scoped_lock lock(m_batch_mutex);

		GpuWorkItem callback_entry;
		callback_entry.on_complete = std::move(request.on_complete);
		callback_entry.command_buffer = cmd_buffer;
		callback_entry.result = std::move(result);

		m_pending_batch.push_back(std::move(callback_entry));
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

void phx::StandardFileProcessor::SubmitBatchedWork(IAllocator* frame_allocator)
{
	PHX_PROFILE;

	if (m_pending_batch.empty())
		return;

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

	size_t pending_work_index = 0;
	if (!m_free_indices.empty())
	{
		pending_work_index = m_free_indices.back();
		m_free_indices.pop_back();
	}
	else
	{
		pending_work_index = m_inflight_work_slots.size();
		m_inflight_work_slots.emplace_back();
	}

	InFlightWorkItem& inflight_work_item = m_inflight_work_slots[pending_work_index];

	SpanMutable commands_to_submit = AllocateArray<rhi::CmdHandle>(frame_allocator, batches_to_submit.Size());
	for (size_t i = 0; i < batches_to_submit.Size(); ++i)
	{
		PendingCallback& pending_callback = inflight_work_item.callbacks.emplace_back();
		pending_callback.on_complete = std::move(batches_to_submit[i].on_complete);
		pending_callback.result = batches_to_submit[i].result;
		commands_to_submit[i] = batches_to_submit[i].command_buffer;
	}
	
	inflight_work_item.fence = rhi::Submit(rhi::CommandQueueType::Copy, commands_to_submit, {});
	m_inflight_indices.push_back(pending_work_index);
}

void phx::StandardFileProcessor::PullCompletions()
{
	for (size_t inflight_index : m_inflight_indices)
	{
		InFlightWorkItem& inflight_work = m_inflight_work_slots[inflight_index];

		if (!rhi::IsFenceCompleted(inflight_work.fence))
			continue;

		for (auto& pending_callback: inflight_work.callbacks)
		{
			JobSystem::SubmitJob(
				[pc = std::move(pending_callback)](JobContext const&) mutable
				{
					pc.on_complete(pc.result);
				},
				JobSystem::Priority::Low);
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
	bool& gpu_operation,
	rhi::CmdHandle& out_cmd_buffer)
{
	// TODO: Check for overrun.
	const std::byte* src_ptr = nullptr;
	void* dest_ptr = nullptr;

	rhi::StagingBlock staging_block;
	
	// Collect Desitantion ptr;
	std::visit([&](auto&& target) {
		using TTarget = std::decay_t<decltype(target)>;
		if constexpr (std::is_same_v<TTarget, CpuDestination>)
		{
			dest_ptr = static_cast<std::byte*>(target.address);
		}
		else if constexpr (	std::is_same_v<TTarget, GpuBufferDestination> ||
							std::is_same_v<TTarget, GpuTextureDestination>)
		{
			// Begin the command buffer if we haven't already
			gpu_operation = true;
			staging_block = rhi::RequestStagingMemory(source_info.size);
			dest_ptr = staging_block.data_ptr;
		}
	}, destination_info.target);

	// copy to the dest position.
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

	if (gpu_operation)
	{
		if (out_cmd_buffer == ~0u)
			out_cmd_buffer = rhi::BeginCommandBuffer(rhi::CommandQueueType::Copy);

		std::visit([&](auto&& arg) {
			using THandle = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<THandle, GpuBufferDestination>)
			{
				rhi::GpuBarrier pre_copy_barrier = rhi::GpuBarrier::CreateBuffer(
					arg.handle,
					rhi::ResourceStates::Common,
					rhi::ResourceStates::CopyDest);

				rhi::InsertBarriers(out_cmd_buffer, { pre_copy_barrier });

				rhi::CopyBuffer(
					out_cmd_buffer,
					staging_block.buffer_handle,
					staging_block.gpu_offset,
					arg.handle,
					arg.offset,
					destination_info.size);
			}
			else if constexpr (std::is_same_v<THandle, GpuTextureDestination>)
			{
				const rhi::TextureDescriptor* desc = rhi::GetTextureDescriptor(arg.handle);
				const uint32_t target_mip = arg.mip_level;
				const uint32_t layer = arg.array_layer;

				rhi::GpuBarrier pre_copy_barrier = rhi::GpuBarrier::CreateTexture(
					arg.handle,
					rhi::ResourceStates::Common,
					rhi::ResourceStates::CopyDest,
					target_mip,
					layer);

 				rhi::InsertBarriers(out_cmd_buffer, { pre_copy_barrier });

				if (target_mip != rhi::c_remaning_mip_levels)
				{
					uint32_t w = std::max(1u, desc->Width >> target_mip);
					uint32_t h = std::max(1u, desc->Height >> target_mip);
					uint32_t d = std::max(1u, (uint32_t)desc->Depth >> target_mip);

					rhi::CopyBufferToTexture(
						out_cmd_buffer,
						staging_block.buffer_handle,
						staging_block.gpu_offset,
						{ 
							.handle = arg.handle,
							.mip_level = target_mip,
							.array_layer = layer 
						},
						{ 
							.width = w,
							.height = h,
							.depth = d 
						});
				}
				else
				{
					uint64_t current_buffer_offset = staging_block.gpu_offset;

					for (uint32_t m = 0; m < desc->MipLevels; ++m)
					{
						uint32_t w = std::max(1u, desc->Width >> m);
						uint32_t h = std::max(1u, desc->Height >> m);
						uint32_t d = std::max(1u, (uint32_t)desc->Depth >> m);

						rhi::CopyBufferToTexture(
							out_cmd_buffer,
							staging_block.buffer_handle,
							current_buffer_offset,
							{
								.handle = arg.handle,
								.mip_level = m,
								.array_layer = layer
							},
						{
							.width = w,
							.height = h,
							.depth = d
						});

						current_buffer_offset += rhi::GetSurfaceSize(desc->Format, w, h, d);
					}
				}
			}
		}, destination_info.target);
	}

	return ErrorCode::Success;
}

bool phx::StandardFileProcessor::ProcessAsyncResourceDesc(
	AsyncResourceDescriptor& descriptor,
	StreamingSource& source_info,
	void* dest_ptr)
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