#include "PhxResource/PhxResource_pch.h"
#include "DStorageAssetStreamer.h"

#include <PhxCore/StringUtils.h>
#include <PhxCore/ThreadPool.h>

using namespace phx;

namespace
{
	struct Request
	{
		HANDLE EventHandle;
		StreamCallback Callback;
		uint32_t StatusIndex;
		Microsoft::WRL::ComPtr<IDStorageStatusArray> StatusArray;
	};

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

		HRESULT hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_metadataQueue)));
		PHX_ASSERT(SUCCEEDED(hr));

		hr = (g_dsFactory->CreateStatusArray(
			kMaxPendingRequests,
			nullptr,
			IID_PPV_ARGS(&m_statusArray)));

		PHX_ASSERT(SUCCEEDED(hr));

	}

	m_filePool.Initialize(200);
}

phx::DStorageAssetStreamer::~DStorageAssetStreamer()
{
	for (auto& hEvent : m_eventPool)
	{
		CloseHandle(hEvent);
	}

	m_eventPool.clear();
}

StreamFileHandle phx::DStorageAssetStreamer::OpenFile(std::filesystem::path const& path)
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


	return retVal;
}

void phx::DStorageAssetStreamer::CloseFile(StreamFileHandle handle)
{
	// All requests created for this instance are tagged with 'this', so we can
	// cancel any outstanding requests.
	auto* fileImpl = m_filePool.Get<DStorageStreamFile>(handle);
	if (fileImpl)
	{
		m_metadataQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
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

		m_metadataQueue->EnqueueRequest(&r);
	}
	
	size_t statusIndex;
	if (m_statusIdxPool.Allocate(statusIndex))
	{
		m_metadataQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(statusIndex));
	}
	else
	{
		PHX_CORE_WARN("Max requests has been reached");
		statusIndex = ~0ull;
	}

	HANDLE hEvent = RequestEvent();
	m_metadataQueue->EnqueueSetEvent(hEvent);
	m_metadataQueue->Submit();

	Request req = {
		.EventHandle = hEvent,
		.Callback = callback,
		.StatusIndex = static_cast<uint32_t>(statusIndex),
		.StatusArray = m_statusArray
	};

	ThreadPool::SubmitTask(
		[req]
		{
			// todo: this event isn't working.
			DWORD waitResult = WaitForSingleObject(req.EventHandle, INFINITE);
			if (waitResult != WAIT_OBJECT_0)
			{
				PHX_CORE_WARN("DStorage request failed. HR={0}.", waitResult);
			}

			HRESULT result = req.StatusArray->GetHResult(req.StatusIndex);
			if (FAILED(result))
			{
				PHX_CORE_WARN("DStorage request failed. HR={0}.", result);
				return;
			}

			req.Callback();
		},
		ThreadPool::Type::Streaming);
}

HANDLE DStorageAssetStreamer::RequestEvent()
{
	std::scoped_lock _(m_eventMutex);
	if (m_freeEvents.empty())
	{
		HANDLE hEvent = CreateEvent(
			NULL,               // Security attributes (default)
			TRUE,              // Auto-reset event (manual-reset if TRUE)
			FALSE,              // Initial state is non-signaled
			L"StreamingEvent"      // Event name (optional)
		);
		m_eventPool.push_back(hEvent);

		ResetEvent(hEvent);
		return hEvent;
	}

	HANDLE hEvent = m_freeEvents.front();
	m_freeEvents.pop_front();

	ResetEvent(hEvent);
	return hEvent;
}

void DStorageAssetStreamer::DisardEvent(HANDLE event)
{
	std::scoped_lock _(m_eventMutex);
	m_freeEvents.push_back(event);
}