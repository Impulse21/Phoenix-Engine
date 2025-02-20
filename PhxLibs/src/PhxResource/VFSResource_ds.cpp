#include "PhxResource_pch.h"

#include "VFSResource_ds.h"
#include "PhxCore/StringUtils.h"

using namespace phx;

namespace
{
	Microsoft::WRL::ComPtr<IDStorageQueue1> g_dsSystemMemoryQueue;
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


		// Create a DirectStorage queue which will be used to load data into a
		// buffer on the GPU.

		// Create the system memory queue, used for reading data into system memory.
		{
			DSTORAGE_QUEUE_DESC queueDesc{};
			queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
			queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
			queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			queueDesc.Name = "SysMemoryQueue";

			hr = (g_dsFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&g_dsSystemMemoryQueue)));
			PHX_ASSERT(SUCCEEDED(hr));
		}
	}
}

phx::DSResourceFileSystem::DSResourceFileSystem()
{
	{
		std::scoped_lock _(g_initMutex);
		if (g_isInit)
			InitDStorage();
	}

	m_filePool.Initialize(200);
}

FileHandle phx::DSResourceFileSystem::Open(std::filesystem::path const& path)
{
	FileHandle retVal = {};
	if (FileExists(path))
	{
		Microsoft::WRL::ComPtr<IDStorageFile> file;
		HRESULT hr = g_dsFactory->OpenFile(path.wstring().c_str(), IID_PPV_ARGS(&file));
		if (FAILED(hr))
		{
			std::string outMsg;
			StringConvert(path.wstring().c_str(), outMsg);
			PHX_ERROR("The file {0}, could no open.", outMsg);
			return;
		}

		retVal = m_filePool.Allocate();
		FileDS& fileImpl = *m_filePool.Get<FileDS>(retVal);
		fileImpl.DsFile = file;

		hr = (file->GetFileInformation(&fileImpl.FileInfo));

		PHX_ASSERT(SUCCEEDED(hr));
		uint32_t fileSize = fileImpl.FileInfo.nFileSizeLow;

		PHX_INFO("[DS] Opened File: {0} [size ={1}] bytes.", path.generic_string().c_str(), fileImpl.FileInfo);

#if false
		hr = (g_dsFactory->CreateStatusArray(
			static_cast<uint32_t>(StatusArrayEntry::NumEntries),
			nullptr,
			IID_PPV_ARGS(&m_statusArray)));

		PHX_ASSERT(SUCCEEDED(hr));

		m_status.store(0xFF);
#endif
	}

	return retVal;
}

void phx::DSResourceFileSystem::Close(FileHandle handle)
{
	// All requests created for this instance are tagged with 'this', so we can
	// cancel any outstanding requests.
	FileDS* fileImpl = m_filePool.Get<FileDS>(handle);
	if (fileImpl)
	{
		g_dsSystemMemoryQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(fileImpl));
	}

	m_filePool.Free(handle);
}

void phx::DSResourceFileSystem::EnqueueRead(FileHandle handle, uint64_t offset, uint64_t size, void* dest, RequestCallbackFunc&& callback)
{
}

std::unique_ptr<IBlob> phx::DSResourceFileSystem::EnqueueReadBlob(FileHandle handle, uint64_t offset, uint64_t size, RequestCallbackFunc&& callback)
{
	return std::unique_ptr<IBlob>();
}

void phx::DSResourceFileSystem::SubmitRequests()
{
}
