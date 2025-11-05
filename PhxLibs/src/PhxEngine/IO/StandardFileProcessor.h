#pragma once

#include "IIoProcessor.h"

#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxEngine/StreamingDefintions.h>

#include <PhxRhi/RHICommon.h>

#include <deque>
#include <mutex>
namespace phx
{
	class StandardFileProcessor final : public IIoProcessor
	{
		struct PendingCallback
		{
			std::function<void(StreamingResult const&)> on_complete;
			StreamingResult result;
		};

		// One fence for a whole batch of callbacks
		struct GpuPendingWork
		{
			rhi::FenceHandle fence;
			std::vector<PendingCallback> callbacks;
		};


	public:
		StandardFileProcessor() = default;
		~StandardFileProcessor() override = default;

	public:
		void ProcessRequest(StreamingRequest&& request) override;
		void SubmitBatchedWork() override;
		void PullCompletions() override;

	public:
		platform::PlatformFileHandle FindOrCreateHandle(std::string const& file_path);

	private:
		ErrorCode ProcessStreamingTransfer(
			StreamingSource& source_info,
			StreamingDestination& destination_info,
			bool& gpu_operation);

		bool ProcessAsyncResourceDesc(
			AsyncResourceDescriptor& descriptor,
			StreamingSource& source_info,
			std::byte* dest_ptr);

	private:

		std::unordered_map<std::string, platform::PlatformFileHandle> m_file_handle_cache;
		std::mutex m_file_handle_cache_mutex;

		std::mutex m_batch_mutex;
		rhi::CommandBufferHandle m_wip_cmd_list;
		std::vector<PendingCallback> m_wip_callbacks;

		std::mutex m_gpu_work_mutex;
		std::vector<GpuPendingWork*> m_pending_gpu_work;
		std::deque<GpuPendingWork*> m_free_gpu_work_pool;
	};
}

