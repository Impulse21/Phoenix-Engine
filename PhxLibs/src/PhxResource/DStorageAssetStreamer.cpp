#include "PhxResource/PhxResource_pch.h"
#include "DStorageAssetStreamer.h"

#include <PhxCore/StringUtils.h>

#include <PhxRhi/d3d12/D3D12Base.h>
#include <PhxRhi/d3d12/D3D12Core.h>

using namespace phx;
using namespace phx::rhi::d3d12;
namespace
{
	Microsoft::WRL::ComPtr<IDStorageFactory> g_dsFactory;
	bool g_isInit = false;
	std::mutex g_initMutex;

	void InitDStorage()
	{
		// Load a file and init D3D12
		HRESULT hr = DStorageGetFactory(IID_PPV_ARGS(&g_dsFactory));
		PHX_ASSERT(SUCCEEDED(hr));
		g_dsFactory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
		g_dsFactory->SetStagingBufferSize(256 * 1024 * 1024);
	}

	constexpr size_t RetrieveFileSize(DWORD sizeLow, DWORD sizeHigh)
	{
		return (static_cast<size_t>(sizeHigh) << sizeof(sizeLow) * 8) | sizeLow;
	}
	constexpr size_t RetrieveFileSize(BY_HANDLE_FILE_INFORMATION fileInfo)
	{
		return RetrieveFileSize(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
	}
}

phx::DStorageAssetStreamer::DStorageAssetStreamer()
{
	{
		std::scoped_lock _(g_initMutex);
		if (!g_isInit)
			InitDStorage();
	}

	// Create the system memory queue, used for reading data into system memory.
	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Name = "SysMetadataQueue";

		HRESULT hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_metadataQueue.Queue)));
		PHX_ASSERT(SUCCEEDED(hr));

		ThrowIfFailed(
			g_d3d12Device->CreateFence(m_metadataQueue.FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_metadataQueue.Fence)));
		m_metadataQueue.Event.Attach(CreateEvent(nullptr, false, false, nullptr));
	}

	m_filePool.Initialize(200);

	m_alive = true;
	m_queueThread = std::thread([this] {
		while (m_alive)
		{
			{
				std::scoped_lock lock(m_swapMutex);
				std::swap(m_processingBatches, m_pendingBatches);
			}

			ProcessBatches();
#if false
			std::unique_lock<std::mutex> lock(m_wakeMutex);
			m_wakeCondition.wait(lock);
#endif
		}
	});

	// TODO:
#ifdef _WIN32
	HANDLE handle = (HANDLE)m_queueThread.native_handle();
	BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
	assert(priorityResult != 0);

	std::wstringstream wss;
	wss << "[PHX] AssetStreamer ";
	HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
	assert(SUCCEEDED(hr));
#endif
}

phx::DStorageAssetStreamer::~DStorageAssetStreamer()
{
	m_alive.store(false);

	bool wakeLoop = true;

	std::thread waker([&] {
		while (wakeLoop)
		{
			m_wakeCondition.notify_all();
		}
	});

	m_queueThread.join();
	wakeLoop = false;
	waker.join();
}

StreamFileHandle phx::DStorageAssetStreamer::OpenFile(std::filesystem::path const& path, uint32_t statusCount)
{
	std::filesystem::path resolvedPath = path;
	StreamFileHandle retVal = {};
	Microsoft::WRL::ComPtr<IDStorageFile> file;
	HRESULT hr = g_dsFactory->OpenFile(resolvedPath.wstring().c_str(), IID_PPV_ARGS(&file));

	if (FAILED(hr))
	{
		std::string outMsg;
		StringConvert(resolvedPath.wstring().c_str(), outMsg);
		PHX_ERROR("The file {0}, could no open.", outMsg);
		return retVal;
	}

	retVal = m_filePool.Allocate();
	auto& fileImpl = *m_filePool.Get<DStorageStreamFile>(retVal);
	fileImpl.DsFile = file;

	hr = (file->GetFileInformation(&fileImpl.FileInfo));
	PHX_ASSERT(SUCCEEDED(hr));

	PHX_INFO("[DStorage] Opened File: {0} [size {1}] bytes.", resolvedPath.generic_string().c_str(), RetrieveFileSize(fileImpl.FileInfo));

	hr = (g_dsFactory->CreateStatusArray(
		static_cast<uint32_t>(statusCount),
		nullptr,
		IID_PPV_ARGS(&fileImpl.StatusArray)));

	PHX_ASSERT(SUCCEEDED(hr));

	return retVal;
}

void phx::DStorageAssetStreamer::CloseFile(StreamFileHandle handle)
{
	// All requests created for this instance are tagged with 'this', so we can
	// cancel any outstanding requests.
	auto* fileImpl = m_filePool.Get<DStorageStreamFile>(handle);
	if (fileImpl)
	{
		m_metadataQueue.Queue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
	}

	PHX_INFO("[DStorage] Closing file handle");
	m_filePool.Free(handle);
}

bool phx::DStorageAssetStreamer::GetStatus(StreamFileHandle /*handle*/, uint32_t /*statusId*/) const
{
#if true
	return true;
#else
	auto* fileImpl = m_filePool.Get<DStorageStreamFile>(handle);
	if (!fileImpl)
		return false;

	uint32_t status = fileImpl->StatusArray->GetHResult(statusId);

	return SUCCEEDED(status);
#endif
}

uint64_t phx::DStorageAssetStreamer::GetFileSize(StreamFileHandle handle) const
{
	auto* fileImpl = m_filePool.Get<DStorageStreamFile>(handle);
	if (!fileImpl)
		return 0;

	return RetrieveFileSize(fileImpl->FileInfo);
}

void phx::DStorageAssetStreamer::SubmitBatch(Span<StreamRequest> requests, StreamCallback callback)
{
	for (const auto& request : requests)
	{
		auto* fileImpl = m_filePool.Get<DStorageStreamFile>(request.FileHandle);
		if (!fileImpl)
			continue;

		DSTORAGE_REQUEST r = {};
		r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
		r.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
		r.Source.File.Source = fileImpl->DsFile.Get();
		r.Source.File.Offset = request.Offset;
		r.Source.File.Size = static_cast<uint32_t>(request.Size);
		r.Destination.Memory.Buffer = request.Destination;
		r.Destination.Memory.Size = r.Source.File.Size;
		r.UncompressedSize = r.Destination.Memory.Size;
		r.CancellationTag = reinterpret_cast<uint64_t>(fileImpl);

		m_metadataQueue.Queue->EnqueueRequest(&r);
	}

	Batch batch = {};
	batch.FenceValue = m_metadataQueue.Submit();
	batch.Callback = callback;

	{
		std::scoped_lock _(m_swapMutex);
		m_pendingBatches.push_back(std::move(batch));
	}

	m_wakeCondition.notify_all();
}

void phx::DStorageAssetStreamer::ProcessBatches()
{
	while (!m_processingBatches.empty())
	{
		Batch& batch = m_processingBatches.front();

		if (m_metadataQueue.Fence->GetCompletedValue() >= batch.FenceValue)
		{
			batch.Callback();
			m_processingBatches.pop_front();
		}
	}
}

inline uint64_t phx::DStorageQueue::Submit()
{
	std::scoped_lock _(SubmitMutex);

	uint64_t fenceValue = ++FenceValue;
	Queue->EnqueueSignal(Fence.Get(), fenceValue);
	ThrowIfFailed(Fence->SetEventOnCompletion(fenceValue, Event.Get()));
	Queue->Submit();

	return fenceValue;
}
