#pragma once

#include "IAssetStreamer.h"

#include <PhxCore/Pool.h>

#include <deque>
#include <wrl.h>
#include <dstorage.h>

namespace phx
{
	struct DStorageStreamFile
	{
		Microsoft::WRL::ComPtr<IDStorageFile> DsFile;
		BY_HANDLE_FILE_INFORMATION FileInfo = {};
		Microsoft::WRL::ComPtr<IDStorageStatusArray> StatusArray;
	};

	class DStorageAssetStreamer final : public IAssetStreamer
	{
	public:
		DStorageAssetStreamer();
		~DStorageAssetStreamer();
		StreamFileHandle OpenFile(std::filesystem::path const& path, uint32_t statusCount) override;
		void CloseFile(StreamFileHandle handle) override;

		bool GetStatus(StreamFileHandle handle, uint32_t status) const override;
		uint64_t GetFileSize(StreamFileHandle handle) const override;

		void SubmitBatch(Span<StreamRequest> requests, StreamCallback callback) override;

	private:
		HANDLE RequestEvent();
		void DisardEvent(HANDLE event);

	private:
		std::mutex m_eventMutex;
		std::vector<HANDLE> m_eventPool;
		std::deque<HANDLE> m_freeEvents;

		Microsoft::WRL::ComPtr<IDStorageQueue1> m_metadataQueue;

		phx::PagedPool<StreamFile, DStorageStreamFile> m_filePool;
	};
}

