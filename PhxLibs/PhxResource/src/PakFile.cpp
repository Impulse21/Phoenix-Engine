#include "PhxResource_pch.h"

#include <PhxResource/PakFile.h>

using namespace phx;

#define ENABLE_DEBUG_PRINT_STRING_ENTRIES 0
#define ENABLE_DEBUG_PRINT_ENTRIES 0

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
#if false
PakFile::PakFile(std::shared_ptr<IAssetStreamer> assetStreamer, std::filesystem::path const& filePath, std::filesystem::path const& resolvedFilePath)
	: Resource("PakFile"_hash)
	, m_filePath(filePath)
	, m_cachedFilename(m_filePath.generic_string())
	, m_resolvedFilePath(resolvedFilePath)
	, m_cachedResolvedFilename(m_resolvedFilePath.generic_string())
	, m_assetStreamer(std::move(assetStreamer))
	, m_status(Status_UnLoaded)
{
	m_fileHandle = m_assetStreamer->OpenFile(m_resolvedFilePath);
}

void phx::PakFile::StartMetadataLoad()
{
	std::memset(&m_header, 0, sizeof(m_header));
	m_assetStreamer->Submit({
			.DebugName = "Metadata Load",
			.FileHandle = m_fileHandle,
			.SrcSize = sizeof(PakFileFormat::Header),
			.DestSize = sizeof(PakFileFormat::Header),
			.Destination = { .Memory = &m_header }
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
		const FileFormat::StringEntry& entry = m_metadata->StringEntries.Get()[mid];

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
	StreamRequest request = StreamRequest::Create<PakFileFormat::MetadataHeader>(
		m_fileHandle,
		sizeof(PakFileFormat::Header),
		m_header.MetadataHeapSize, m_metadata);
	m_assetStreamer->Submit(request, [this] { OnMetadataLoaded(); });

	m_status = 0xf0;
}

void phx::PakFile::OnMetadataLoaded()
{
	m_status = 0;
	PakFileFormat::MetadataHeader* metadata = m_metadata.Get();

	PHX_CORE_INFO("Metadata Loading Completed - There are {0} entries and {1} strings", metadata->NumEntries, metadata->NumStrings);
#if ENABLE_DEBUG_PRINT_ENTRIES
	for (size_t i = 0; i < metadata->NumEntries; i++)
	{
		auto& assetEntry = metadata->AssetEntries.Get()[i];
		char buffer[9]; // 8 characters + null terminator
		std::snprintf(buffer, sizeof(buffer), "%08X", assetEntry.Hash);
		PHX_CORE_INFO("Asset Entry {0} - Hash={1}", i, buffer);
	}
#endif
#if ENABLE_DEBUG_PRINT_STRING_ENTRIES
	for (size_t i = 0; i < metadata->NumStrings; i++)
	{
		auto& stringEntry = metadata->StringEntries.Get()[i];
		char buffer[9]; // 8 characters + null terminator
		std::snprintf(buffer, sizeof(buffer), "%08X", stringEntry.Hash);
		PHX_CORE_INFO("STRING TABLE Entry {0} - Hash={1} Value={2}", i, buffer, stringEntry.Value.Get());
	}
#endif
}
#endif