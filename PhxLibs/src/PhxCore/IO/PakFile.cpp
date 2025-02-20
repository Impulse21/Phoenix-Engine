#include "PhxCore/PhxCore_pch.h"

#include "PakFile.h"
#if false
#include "PhxCore/StringUtils.h"
#include <wrl.h>

using namespace phx;


namespace phx
{
	Microsoft::WRL::ComPtr<IDStorageQueue1> g_dsSystemMemoryQueue;
	Microsoft::WRL::ComPtr<IDStorageFactory> g_dsFactory;

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

PakFile::PakFile(std::filesystem::path const& path)
	: m_headerLoaded(EventWait::Create<PakFile, &PakFile::OnHeaderLoaded>(this))
	, m_assetIndexLoaded((EventWait::Create<PakFile, &PakFile::OnAssetIndexLoaded>(this)))
{
	HRESULT hr = g_dsFactory->OpenFile(path.wstring().c_str(), IID_PPV_ARGS(&m_dsFile));
	if (FAILED(hr))
	{
		std::string outMsg;
		StringConvert(path.wstring().c_str(), outMsg);
		PHX_ERROR("The file {0}, could no open.", outMsg);
		return;
	}
	hr = (m_dsFile->GetFileInformation(&m_fileInfo));
	PHX_ASSERT(SUCCEEDED(hr));
	uint32_t fileSize = m_fileInfo.nFileSizeLow;
	PHX_INFO("File Size {0}.", fileSize);

	hr = (g_dsFactory->CreateStatusArray(
		static_cast<uint32_t>(StatusArrayEntry::NumEntries),
		nullptr,
		IID_PPV_ARGS(&m_statusArray)));

	PHX_ASSERT(SUCCEEDED(hr));

	m_status.store(0xFF);
}

PakFile::~PakFile()
{
	// All requests created for this instance are tagged with 'this', so we can
	// cancel any outstanding requests.
	g_dsSystemMemoryQueue->CancelRequestsWithTag(0xFFFFFFFFFFFFll, reinterpret_cast<uint64_t>(this));
}

void PakFile::StartMetadataLoad()
{
	std::scoped_lock _(m_mutex);

	ValidateState(InternalState::FileOpen);

	EnqueueRead(0, &m_header);

	m_headerLoaded.SetThreadpoolWait();
	g_dsSystemMemoryQueue->EnqueueStatus(m_statusArray.Get(), static_cast<uint32_t>(StatusArrayEntry::Metadata));
	g_dsSystemMemoryQueue->EnqueueSetEvent(m_headerLoaded);
	g_dsSystemMemoryQueue->Submit();

	m_state = InternalState::LoadingHeader;
	m_status = PakStatus::LoadingHeader;
}

const char* PakFile::FindFilenameByHash(phx::StringHash targetHash)
{
	size_t left = 0;
	size_t right = m_header.NumStrings - 1;

	while (left <= right)
	{
		size_t mid = left + (right - left) / 2;
		const PakFileFormat::StringEntry& entry = m_assetStringEntriesData.Get()[mid];

		if (entry.Hash == targetHash)
		{
			return m_assetStringHeap.Get() + entry.Offset; // Return pointer to the filename
		}
		else if (entry.Hash < targetHash)
		{
			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}
	}

	return nullptr; // Not found
}

void PakFile::OnHeaderLoaded()
{
	std::scoped_lock _(m_mutex);

	ValidateState(InternalState::LoadingHeader);

	uint32_t status = m_statusArray->GetHResult(static_cast<uint32_t>(StatusArrayEntry::Metadata));

	if (m_header.Magic != PakFileFormat::MagicNumber ||
		m_header.Version != PakFileFormat::Version ||
		FAILED(status))
	{
		m_state = InternalState::Error;
		return;
	}

	const size_t fileSize = (static_cast<size_t>(m_fileInfo.nFileSizeHigh) << sizeof(m_fileInfo.nFileSizeLow) * 8) | m_fileInfo.nFileSizeLow;

	const uint32_t sizeOfEntries = static_cast<uint32_t>(m_header.NumEntries * sizeof(PakFileFormat::AssetEntry));
	m_assetEntriesData = EnqueueReadMemoryRegion<PakFileFormat::AssetEntry>(m_header.EntriesOffset, sizeOfEntries);

	const uint32_t sizeOfStringEntries = static_cast<uint32_t>(m_header.NumStrings * sizeof(PakFileFormat::StringEntry));
	const size_t stringTableOffset = sizeof(PakFileFormat::Header) + sizeOfEntries;
	m_assetStringEntriesData = EnqueueReadMemoryRegion<PakFileFormat::StringEntry>(stringTableOffset, sizeOfStringEntries);
	m_assetStringHeap = EnqueueReadMemoryRegion<char>(fileSize - m_header.StringHeapSize, m_header.StringHeapSize);

	m_assetIndexLoaded.SetThreadpoolWait();
	g_dsSystemMemoryQueue->EnqueueSetEvent(m_assetIndexLoaded);
	g_dsSystemMemoryQueue->Submit();

	m_state = InternalState::LoadingAssetIndex;
	m_status = PakStatus::LoadingAssetHeaders;
}

void PakFile::OnAssetIndexLoaded()
{
	std::scoped_lock _(m_mutex);

	ValidateState(InternalState::LoadingAssetIndex);
	m_status = PakStatus::Loaded;

}
#endif