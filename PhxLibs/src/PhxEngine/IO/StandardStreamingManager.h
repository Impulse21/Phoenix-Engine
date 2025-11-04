#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxEngine/IStreamingManager.h>
#include <PhxRhi/PhxRhi.h>

#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{
	namespace rhi
	{
		class GfxDevice;
	}
}
namespace phx
{
	class StandardStreamingManager final : public IStreamingManager
	{
		struct PendingCallback
		{
			std::function<void(StreamingResult const&)> on_complete;
			StreamingResult result;
		};

		// One fence for a whole batch of callbacks
		struct GpuPendingWork
		{
			uint64_t fence;
			std::vector<PendingCallback> callbacks;
		};

		// Holds temporary data for processing a single request's operations
		struct ProcessingState;

	public:
		StandardStreamingManager(IVirtualFileSystem* vfs)
			: m_vfs(vfs)
		{
		}

		~StandardStreamingManager() override
		{
			Shutdown();
		}

		void Initialize() override;
		void Shutdown() override;

		void Submit(StreamingRequest&& request) override;
		void SubmitStreamingCopies() override;
		void PollGpuCompletions() override;

	public:
		platform::PlatformFileHandle FindOrCreateHandle(std::string const& file_path);

	private:
		void StreamingThreadLoop();

		ErrorCode ProcessOperation(
			RHI::CommandBufferHandle ctx_handle,
			ProcessingState& state,
			StreamingOperation& operation);

		template<typename TSource>
		ErrorCode ProcessStreamingTransfer(
			RHI::CommandBufferHandle ctx_handle,
			ProcessingState& state,
			TSource& source_data,
			StreamingSource& source_info,
			StreamingDestination& destination_info);

		bool ProcessAsyncResourceDesc(
			ProcessingState& state,
			AsyncResourceDescriptor& descriptor,
			StreamingSource& source_info);
	private:
		IVirtualFileSystem* m_vfs = nullptr;

		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;

		std::deque<StreamingRequest> m_requestQueue;
		std::mutex m_queueMutex;

		std::unordered_map<std::string, platform::PlatformFileHandle> m_fileHandleCache;
		std::mutex m_fileHandleCacheMutex;

		std::mutex m_batch_mutex;
		RHI::CommandBufferHandle m_wip_cmd_list;
		std::vector<PendingCallback> m_wip_callbacks;

		std::mutex m_gpu_work_mutex;
		std::vector<GpuPendingWork*> m_pending_gpu_work;
		std::deque<GpuPendingWork*> m_free_gpu_work_pool;
	};

	template<typename TSource>
	ErrorCode StandardStreamingManager::ProcessStreamingTransfer(
		RHI::CommandBufferHandle ctx_handle,
		ProcessingState& state,
		TSource& source_data,
		StreamingSource& source_info,
		StreamingDestination& destination_info)
	{
		// 1. Process the source to get data_to_transfer
		if constexpr (std::is_same_v<std::decay_t<TSource>, AsyncResourceDescriptor>)
		{
			if (!ProcessAsyncResourceDesc(state, source_data, source_info)) {
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

			state.data_to_transfer = reinterpret_cast<const std::byte*>(buffer.get()) + source_info.offset;
			state.effective_transfer_size = source_info.size;
		}
		else {
			PHX_CORE_ERROR("Unknown Source Type!");
			return ErrorCode::Unknown;
		}

		// 2. Process the destination
		ErrorCode ret_val = ErrorCode::Unknown;
		std::visit([&](auto&& dest_active_type)
			{
				using TDest = std::decay_t<decltype(dest_active_type)>;

				if constexpr (std::is_same_v<TDest, CpuResourceDestinationInfo>)
				{
					CpuResourceDestinationInfo& cpu_dest_info = dest_active_type;
					auto [dest_ptr, error] = GetCpuDestinationPointer(
						cpu_dest_info.handle,
						destination_info.offset,
						destination_info.size,
						state.effective_transfer_size
					);

					if (error == ErrorCode::Success) {
						std::memcpy(dest_ptr, state.data_to_transfer, state.effective_transfer_size);
						ret_val = ErrorCode::Success;
					}
					else {
						ret_val = error;
					}
				}
				else if constexpr (std::is_same_v<TDest, GpuResourceDestinationInfo>)
				{
					GpuResourceDestinationInfo& gpu_dest_info = dest_active_type;
					std::visit([&](auto&& handle_type) {
						using THandle = std::decay_t<decltype(handle_type)>;

						if constexpr (std::is_same_v<THandle, RHI::GpuBufferHandle>)
						{
							// --- THIS IS THE GPU UPLOAD LOGIC ---
#if false
							rhi::StagingBlock staging_block =
								RHI::AllocateStagingMemory(ctx_handle, state.effective_transfer_size);

							std::memcpy(staging_block.pCpuAddress, state.data_to_transfer, state.effective_transfer_size);

#
							RHI::CmdCopyBuffer(
								ctx_handle,
								staging_block.gpuBufferHandle,
								staging_block.offsetInBufer,
								handle_type,
								destination_info.offset,
								state.effective_transfer_size
							);
#endif
							ret_val = ErrorCode::Success;
						}
						else if constexpr (std::is_same_v<THandle, RHI::TextureHandle>) {
							// TODO: Texture uploading logic
							ret_val = ErrorCode::Success; // Placeholder
						}
						else {
							ret_val = ErrorCode::Unknown;
						}
						}, gpu_dest_info.handle);
				}
			},
			destination_info.target);

		return ret_val;
	}
}

