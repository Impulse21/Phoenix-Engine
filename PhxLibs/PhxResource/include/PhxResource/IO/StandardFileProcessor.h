#pragma once

#include "IIoProcessor.h"

#include <PhxCore/Platform/Platform.h>

#include <PhxResource/IO/StreamingDefintions.h>

#include <PhxRhi/PhxRhi.h>

#include <vector>
#include <deque>
#include <mutex>

namespace phx
{
	class StandardFileProcessor final : public IIoProcessor
	{
		// One fence for a whole batch of callbacks
		struct GpuWorkItem
		{
			rhi::CmdHandle command_buffer;
			std::function<void(StreamingResult const&)> on_complete;
			StreamingResult result;
		};

		struct PendingCallback
		{
			std::function<void(StreamingResult const&)> on_complete;
			StreamingResult result;
		};

		struct InFlightWorkItem
		{
			rhi::FenceHandle fence;
			std::vector<PendingCallback> callbacks;
		};


	public:
		StandardFileProcessor() = default;
		~StandardFileProcessor() override;

	public:
		void ProcessRequest(StreamingRequest&& request) override;
		void SubmitBatchedWork(IAllocator* frame_allocator) override;
		void PullCompletions() override;

	public:
		PlatformFileHandle FindOrCreateHandle(std::string const& file_path);

	private:
		ErrorCode ProcessStreamingTransfer(
			StreamingSource& source_info,
			StreamingDestination& destination_info,
			bool& gpu_operation,
			rhi::CmdHandle& out_cmd_buffer);

		bool ProcessAsyncResourceDesc(
			AsyncResourceDescriptor& descriptor,
			StreamingSource& source_info,
			void* dest_ptr);

	private:
		std::unordered_map<std::string, PlatformFileHandle> m_file_handle_cache;
		std::mutex m_file_handle_cache_mutex;

		std::mutex m_batch_mutex;
		std::vector<GpuWorkItem> m_pending_batch;

		// These are local to one thread
		std::vector<InFlightWorkItem>	m_inflight_work_slots;
		std::vector<size_t>				m_inflight_indices;
		std::deque<size_t>				m_free_indices;
	};
}

