#include "PhxResource/PhxResource_pch.h"

#include "DStorageAssetStreamer.h"

#ifdef PHX_RHI_D3D12
#include <PhxCore/StringUtils.h>
#include <PhxCore/ThreadPool.h>

#ifdef PHX_RHI_D3D12
#include <PhxRhi/d3d12/D3D12Core.h>
#endif

using namespace phx;

namespace
{
	EnumArray<DSTORAGE_REQUEST_DESTINATION_TYPE, phx::DestinationType, 5> kDsConvertDestType = {
		DSTORAGE_REQUEST_DESTINATION_MEMORY,
		DSTORAGE_REQUEST_DESTINATION_BUFFER,
		DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION,
		DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES,
		DSTORAGE_REQUEST_DESTINATION_TILES
	};

	EnumArray<DSTORAGE_COMPRESSION_FORMAT, phx::FileFormat::CompressionType, 2> kDsCompressionFormat = {
		DSTORAGE_COMPRESSION_FORMAT_NONE,
		DSTORAGE_COMPRESSION_FORMAT_GDEFLATE,
	};

	struct Request
	{
		std::array<HANDLE, 2> EventHandles;
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

	bool IsDStorageError(HRESULT hr)
	{
		return (hr & 0xFFFF0000) == 0x89240000;
	}

	const std::unordered_map<HRESULT, const char*> DStorageErrorMessages =
	{
		{ 0x89240001, "DStorage is already running exclusively" },
		{ 0x89240002, "DStorage is not running" },
		{ 0x89240003, "Invalid queue capacity parameter" },
		{ 0x89240007, "Offset and length exceed file size" },
		{ 0x89240008, "IO request too large" },
		{ 0x89240009, "Access violation - buffer not accessible" },
		{ 0x8924000B, "File is not open" },
		{ 0x89240010, "Queue is closed" },
		{ 0x89240012, "Too many queues" },
		{ 0x89240014, "Too many files" },
		{ 0x89240016, "IO operation timed out" },
		{ 0x89240017, "Invalid file handle" },
		{ 0x89240021, "Staging buffer too small" },
		{ 0x89240030, "Generic decompression error" },
	};
	
	const char* GetDStorageErrorMessage(HRESULT hr)
	{
		PHX_ASSERT(IsDStorageError(hr));
		auto it = DStorageErrorMessages.find(hr);
		return it != DStorageErrorMessages.end() ? it->second : "Unknown DStorage error";
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

		HRESULT hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_dsMetadataQueue)));
		PHX_ASSERT(SUCCEEDED(hr));
	}

	// Create the GPU queue, used for reading GPU resources.
	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Device = RHI::d3d12::g_d3d12Device.Get();
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Name = "g_dsGpuQueue";

		HRESULT hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_dsGpuQueue)));
		PHX_ASSERT(SUCCEEDED(hr));
	}

	// Status array
	HRESULT hr = (g_dsFactory->CreateStatusArray(
		kMaxPendingRequests,
		nullptr,
		IID_PPV_ARGS(&m_statusArray)));

	PHX_ASSERT(SUCCEEDED(hr));

	// TODO: Custom Decompression Queue. Looks like this uses MS thread pool.
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
		m_dsMetadataQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
		m_dsGpuQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
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
		r.Options.DestinationType = kDsConvertDestType[request.Destination.Type];
		r.Options.CompressionFormat = kDsCompressionFormat[request.CompressionType];

		r.Source.File.Source = fileImpl->DsFile.Get();
		r.Source.File.Offset = request.Offset;
		r.Source.File.Size = static_cast<uint32_t>(request.SrcSize);

		switch (request.Destination.Type)
		{
		case DestinationType::Memory:
		{
			r.Destination.Memory.Buffer = request.Destination.Memory;
			r.Destination.Memory.Size = request.DestSize;
			break;
		}
		case DestinationType::RHI_GpuBuffer:
		{
			// Request
			auto buffer = RHI::d3d12::g_bufferPool.Get<RHI::d3d12::GpuBuffer>(request.Destination.Buffer);
			if (!buffer)
			{
				PHX_CORE_ERROR("Cannot make buffer request '{}' as buffer handle is invalid.", request.DebugName);
				continue;
			}

			r.Destination.Buffer.Offset = 0;
			r.Destination.Buffer.Resource = buffer->Resource.Get();
			r.Destination.Buffer.Size = request.DestSize;
			break;
		}
		case DestinationType::RHI_Texture:
		case DestinationType::RHI_Multi_Subresource:
		case DestinationType::RHI_TiledTexture:
		default:
			PHX_ASSERT(false, "TODO: Implementat");
		}

		r.UncompressedSize = request.DestSize;
		r.CancellationTag = reinterpret_cast<uint64_t>(fileImpl);

		if (request.Destination.Type == DestinationType::Memory)
			m_dsMetadataQueue->EnqueueRequest(&r);
		else
			m_dsGpuQueue->EnqueueRequest(&r);
	}
	
	size_t statusIndex;
	if (m_statusIdxPool.Allocate(statusIndex))
	{
		m_dsMetadataQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(statusIndex));
		m_dsGpuQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(statusIndex));
	}
	else
	{
		PHX_CORE_WARN("Max requests has been reached");
		statusIndex = ~0ull;
	}

	HANDLE metadataEvent = RequestEvent();
	m_dsMetadataQueue->EnqueueSetEvent(metadataEvent);
	m_dsMetadataQueue->Submit();

	HANDLE gpuEvent = RequestEvent();
	m_dsGpuQueue->EnqueueSetEvent(gpuEvent);
	m_dsGpuQueue->Submit();

	Request req = {
		.EventHandles = { metadataEvent, gpuEvent},
		.Callback = callback,
		.StatusIndex = static_cast<uint32_t>(statusIndex),
		.StatusArray = m_statusArray
	};

	ThreadPool::SubmitTask(
		[this, req]
		{
			// todo: this event isn't working.
			for (HANDLE eventHandle : req.EventHandles)
			{
				DWORD waitResult = WaitForSingleObject(eventHandle, INFINITE);
				if (waitResult != WAIT_OBJECT_0)
				{
					PHX_CORE_ERROR("DStorage wait request failed. HR={0}.", waitResult);
				}

				DisardEvent(eventHandle);
			}

			HRESULT result = req.StatusArray->GetHResult(req.StatusIndex);
			if (FAILED(result))
			{
				PHX_CORE_ERROR(
					"DStorage request failed. HR={0}, Msg={1}.",
					result,
					GetDStorageErrorMessage(result));
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
#endif