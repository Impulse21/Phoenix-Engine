#pragma once

#include <PhxCore/StringHash.h>
#include<PhxCore/IO/MemoryRegion.h>

#include "PakFileFormat.h"
#include "IResource.h"
#include "IAssetStreamer.h"

namespace phx
{
	class PakFile : public RefCounter<IResource>
	{
	public:
		PakFile(std::shared_ptr<IAssetStreamer> assetStreamer, std::filesystem::path const& path);
		~PakFile()
		{
			m_assetStreamer->CloseFile(m_fileHandle);
		}

		void StartMetadataLoad();

		bool IsLoaded() const { return m_status == Status_Loaded; }

		StreamFileHandle GetFileHandle() const { return m_fileHandle; }
		const std::filesystem::path& GetFilePath() const { return m_filePath; }
		std::string GetFilename() const { return m_cachedFilename; }
		Span<PakFileFormat::AssetEntry> GetEntries() const { return Span(m_assetEntries.Get(), m_header.NumEntries); }

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

		template<typename T>
		StreamRequest EnqueueReadMemoryRegion(uint64_t offset, uint32_t size, MemoryRegion<T>& outRegion)
		{
			outRegion = MemoryRegion<T>(std::make_unique<char[]>(size));

			return {
				.FileHandle = m_fileHandle,
				.Offset = offset,
				.Size = size,
				.Destination = outRegion.Get()
			};
		}

	private:
		std::filesystem::path m_filePath;
		std::string m_cachedFilename;
		std::shared_ptr<IAssetStreamer> m_assetStreamer;
		StreamFileHandle m_fileHandle;
		std::atomic_uint8_t m_status;

		PakFileFormat::Header m_header;

		MemoryRegion<PakFileFormat::AssetEntry> m_assetEntries;
		MemoryRegion<PakFileFormat::StringEntry> m_assetStringEntriesData;
		MemoryRegion<char> m_assetStringHeap;
	};
}

