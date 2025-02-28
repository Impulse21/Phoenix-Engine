#include "PhxResource/PhxResource_pch.h"
#include "PakFile.h"

using namespace phx;

namespace
{
	namespace StreamingStatus
	{
		enum : uint32_t
		{
			Metadata = 0,
			CpuData,
			GpuData,
			NumEntries
		};
	}
}

PakFile::PakFile(std::shared_ptr<IAssetStreamer> assetStreamer, std::filesystem::path const& filePath, std::filesystem::path const& resolvedFilePath)
	: m_filePath(filePath)
	, m_cachedFilename(m_filePath.generic_string())
	, m_resolvedFilePath(resolvedFilePath)
	, m_cachedResolvedFilename(m_resolvedFilePath.generic_string())
	, m_assetStreamer(std::move(assetStreamer))
	, m_status(Status_UnLoaded)
{
	m_fileHandle = m_assetStreamer->OpenFile(m_resolvedFilePath, StreamingStatus::NumEntries);
}

void phx::PakFile::StartMetadataLoad()
{
	std::memset(&m_header, 0, sizeof(m_header));
	m_assetStreamer->Submit({
			.FileHandle = m_fileHandle,
			.Size = sizeof(PakFileFormat::Header),
			.Destination = &m_header
		},
		[this] {
			OnHeaderLoaded();
		});
}

const char* PakFile::FindFilenameByHash(phx::StringHash targetHash)
{
	size_t left = 0;
	size_t right = m_metadata->NumStrings - 1;

	while (left <= right)
	{
		size_t mid = left + (right - left) / 2;
		const PakFileFormat::StringEntry& entry = m_metadata->StringEntries.Get()[mid];

		if (entry.Hash == targetHash)
		{
			return entry.Value.Get(); // Return pointer to the filename
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

const PakFileFormat::AssetEntry* phx::PakFile::FindEntryByHash(phx::StringHash filename)
{
	size_t left = 0;
	size_t right = m_metadata->NumEntries - 1;

	while (left <= right)
	{
		size_t mid = left + (right - left) / 2;
		const PakFileFormat::AssetEntry& entry = m_metadata->AssetEntries.Get()[mid];

		if (entry.Hash == filename)
		{
			return &entry;
		}
		else if (entry.Hash < filename)
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

void phx::PakFile::OnHeaderLoaded()
{
	bool status = m_assetStreamer->GetStatus(m_fileHandle, StreamingStatus::Metadata);

	if (!status ||
		m_header.Magic != PakFileFormat::MagicNumber ||
		m_header.Version != PakFileFormat::Version)
	{
		return;
	}

#if false
	const size_t fileSize = m_assetStreamer->GetFileSize(m_fileHandle);
	const uint32_t sizeOfEntries = static_cast<uint32_t>(m_header.NumEntries * sizeof(PakFileFormat::AssetEntry));

	std::array<StreamRequest, 3> batchedRequests;
	batchedRequests[0] = EnqueueReadMemoryRegion<PakFileFormat::AssetEntry>(m_header.EntriesOffset, sizeOfEntries, m_assetEntries);

	const uint32_t sizeOfStringEntries = static_cast<uint32_t>(m_header.NumStrings * sizeof(PakFileFormat::StringEntry));
	const size_t stringTableOffset = sizeof(PakFileFormat::Header) + sizeOfEntries;
	batchedRequests[1] = EnqueueReadMemoryRegion<PakFileFormat::StringEntry>(stringTableOffset, sizeOfStringEntries, m_assetStringEntriesData);
	batchedRequests[2] = EnqueueReadMemoryRegion<char>(fileSize - m_header.StringHeapSize, m_header.StringHeapSize, m_assetStringHeap);

	m_assetStreamer->SubmitBatch(batchedRequests, [this] { OnMetadataLoaded(); });
#else

	StreamRequest request = EnqueueReadMemoryRegion<PakFileFormat::MetadataHeader>(sizeof(PakFileFormat::Header), m_header.MetadataHeapSize, m_metadata);
	m_assetStreamer->Submit(request, [this] { OnMetadataLoaded(); });
#endif
	m_status = 0xf0;
}

void phx::PakFile::OnMetadataLoaded()
{
	m_status = 0;
	PakFileFormat::MetadataHeader* metadata = m_metadata.Get();

	PHX_CORE_INFO("Metadata Loading Completed - There are {0} entries and {1} strings", metadata->NumEntries, metadata->NumStrings);
}
