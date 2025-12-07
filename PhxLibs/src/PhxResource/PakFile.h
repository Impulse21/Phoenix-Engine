#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <PhxCore/RefCountPtr.h>

#include "PakFileFormat.h"
#include "Resource.h"
#if false
#include "IAssetStreamer.h"

namespace phx
{
	class PakFile final : public Resource
	{
	public:
		PakFile(std::shared_ptr<IAssetStreamer> assetStreamer, std::filesystem::path const& filePath, std::filesystem::path const& resolvedFilePath);
		~PakFile()
		{
			m_assetStreamer->CloseFile(m_fileHandle);
		}

		void StartMetadataLoad();

		bool IsLoaded() const { return m_status == Status_Loaded; }

		StreamFileHandle GetFileHandle() const { return m_fileHandle; }
		const std::filesystem::path& GetFilePath() const { return m_filePath; }
		std::string GetFilename() const { return m_cachedFilename; }
		const std::filesystem::path& GetResolvedFilePath() const { return m_resolvedFilePath; }
		std::string GetResolvedFilename() const { return m_cachedResolvedFilename; }
		Span<PakFileFormat::AssetEntry> GetEntries() const { return Span(m_metadata->AssetEntries.Get(), m_metadata->NumEntries); }

		const char* FindFilenameByHash(phx::StringHash targetHash);

		const PakFileFormat::AssetEntry* FindEntryByHash(phx::StringHash filename);

		enum Status
		{
			Status_Loaded = 0,
			Status_LoadingMetdata = 0xF0,
			Status_UnLoaded = 0xFF,
		};

	private:
		void OnHeaderLoaded();
		void OnMetadataLoaded();


	private:
		std::filesystem::path m_filePath;
		std::string m_cachedFilename;
		std::filesystem::path m_resolvedFilePath;
		std::string m_cachedResolvedFilename;
		std::shared_ptr<IAssetStreamer> m_assetStreamer;
		StreamFileHandle m_fileHandle;
		std::atomic_uint8_t m_status;

		PakFileFormat::Header m_header;
		MemoryRegion<PakFileFormat::MetadataHeader> m_metadata;

#if false
		MemoryRegion<PakFileFormat::AssetEntry> m_assetEntries;
		MemoryRegion<PakFileFormat::StringEntry> m_assetStringEntriesData;
		MemoryRegion<char> m_assetStringHeap;
#endif
	};
}
#endif
