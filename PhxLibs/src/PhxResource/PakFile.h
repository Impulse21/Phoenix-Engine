#pragma once

#include <PhxCore/StringHash.h>

#include "PakFileFormat.h"
#include "IResource.h"
#include "VFSResource.h"

namespace phx
{
	class PakFile : public RefCounter<IResource>
	{
		friend class PakFileHandler;
	public:
		PakFile(std::filesystem::path const& path, FileHandle handle, std::shared_ptr<IResourceFileSystem> fs);
		~PakFile()
		{
			m_fs->Close(m_fileHandle);
		}

		bool IsLoaded() const { return m_status == Status_Loaded; }

		FileHandle GetFileHandle() const { return m_fileHandle; }
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
		std::filesystem::path m_filePath;
		std::string m_cachedFilename;

		FileHandle m_fileHandle;
		PakFileFormat::Header m_header;
		std::shared_ptr<IResourceFileSystem> m_fs;
		std::atomic_uint8_t m_status;

		MemoryRegion<PakFileFormat::AssetEntry> m_assetEntries;
		MemoryRegion<PakFileFormat::StringEntry> m_assetStringEntriesData;
		MemoryRegion<char> m_assetStringHeap;
	};

	class PakFileHandler
	{
	public:
		RefCountPtr<PakFile> Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs) const;

	};
}

