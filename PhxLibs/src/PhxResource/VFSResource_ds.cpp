#include "PhxResource_pch.h"

#include "VFSResource_ds.h"
#include "PhxCore/StringUtils.h"
#include <wrl/wrappers/corewrappers.h>

using namespace phx;

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

phx::DSResourceFileSystem::DSResourceFileSystem()
{
	{
		std::scoped_lock _(g_initMutex);
		if (!g_isInit)
			InitDStorage();
	}

	m_rootFs = FileSystemFactory::CreateRootFileSystem();

	// Create a DirectStorage queue which will be used to load data into a
	// buffer on the GPU.

	// Create the system memory queue, used for reading data into system memory.
	{
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Name = "SysMemoryQueue";

		HRESULT hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_dsSystemMemoryQueue)));
		PHX_ASSERT(SUCCEEDED(hr));
	}
	m_filePool.Initialize(200);
}

FileHandle phx::DSResourceFileSystem::Open(std::filesystem::path const& path)
{
	std::filesystem::path resolvedPath = ResolvePath(path);
	FileHandle retVal = {};
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
	FileDS& fileImpl = *m_filePool.Get<FileDS>(retVal);
	fileImpl.DsFile = file;

	hr = (file->GetFileInformation(&fileImpl.FileInfo));
	PHX_ASSERT(SUCCEEDED(hr));

	PHX_INFO("[DS] Opened File: {0} [size {1}] bytes.", resolvedPath.generic_string().c_str(), RetrieveFileSize(fileImpl.FileInfo));

	hr = (g_dsFactory->CreateStatusArray(
		static_cast<uint32_t>(255),
		nullptr,
		IID_PPV_ARGS(&fileImpl.StatusArray)));

	PHX_ASSERT(SUCCEEDED(hr));

	return retVal;
}

void phx::DSResourceFileSystem::Close(FileHandle handle)
{
	// All requests created for this instance are tagged with 'this', so we can
	// cancel any outstanding requests.
	FileDS* fileImpl = m_filePool.Get<FileDS>(handle);
	if (fileImpl)
	{
		m_dsSystemMemoryQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
	}

	PHX_INFO("[DS] Closing file handle");
	m_filePool.Free(handle);
}

void phx::DSResourceFileSystem::EnqueueRead(ReadRequest const& request)
{
	FileDS* fileImpl = m_filePool.Get<FileDS>(request.Handle);
	if (!fileImpl)
		return;

	DSTORAGE_REQUEST r{};
	r.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	r.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
	r.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
	r.Source.File.Source = fileImpl->DsFile.Get();
	r.Source.File.Offset = request.Offset;
	r.Source.File.Size = static_cast<uint32_t>(request.Size);
	r.Destination.Memory.Buffer = request.Dest;
	r.Destination.Memory.Size = r.Source.File.Size;
	r.UncompressedSize = r.Destination.Memory.Size;
	r.CancellationTag = reinterpret_cast<uint64_t>(fileImpl);

	// TODO: Set up events
	m_dsSystemMemoryQueue->EnqueueRequest(&r);
}

size_t phx::DSResourceFileSystem::GetFileSize(FileHandle handle) const
{
	const FileDS* fileImpl = m_filePool.Get<FileDS>(handle);
	return fileImpl ? RetrieveFileSize(fileImpl->FileInfo) : 0;
}

void phx::DSResourceFileSystem::SubmitRequests(RequestCallbackFunc&& callback)
{
	m_callbackQueue.EnqueueCallback(std::move(callback));
	m_callbackQueue.SetThreadpoolWait();
	//m_dsSystemMemoryQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(StatusArrayEntry::Metadata));
	m_dsSystemMemoryQueue->EnqueueSetEvent(m_callbackQueue.GetEvent());
	m_dsSystemMemoryQueue->Submit();
}
