#pragma once

#include "IAssetStreamer.h"

#include <PhxCore/Pool.h>

#include <deque>
#include <wrl.h>
#include <dstorage.h>

#include <wrl/wrappers/corewrappers.h>
namespace phx
{
	struct DStorageStreamFile
	{
		Microsoft::WRL::ComPtr<IDStorageFile> DsFile;
		BY_HANDLE_FILE_INFORMATION FileInfo = {};
		Microsoft::WRL::ComPtr<IDStorageStatusArray> StatusArray;
	};

	struct DStorageQueue
	{
		Microsoft::WRL::ComPtr<IDStorageQueue1> Queue;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		Microsoft::WRL::Wrappers::Event Event;
		uint64_t FenceValue = 0;
		std::mutex SubmitMutex;

		inline uint64_t Submit();
	};
	class DStorageAssetStreamer final : public IAssetStreamer
	{
	public:
		DStorageAssetStreamer();
		~DStorageAssetStreamer();
		StreamFileHandle OpenFile(std::filesystem::path const& path) override;
		void CloseFile(StreamFileHandle handle) override;

		void SubmitBatch(Span<StreamRequest> requests, StreamCallback callback) override;

	private:
		void ProcessBatches();
		struct Batch
		{
			uint64_t FenceValue;
			StreamCallback Callback;
		};

	private:
		std::deque<Batch> m_pendingBatches;
		std::mutex m_batchMutex;

		DStorageQueue m_metadataQueue;
		phx::PagedPool<StreamFile, DStorageStreamFile> m_filePool;

		std::thread m_queueThread;
		std::condition_variable m_wakeCondition;
		std::mutex m_wakeMutex;
		std::atomic_bool m_alive = false;
	};
}

