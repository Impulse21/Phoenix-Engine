#include "PhxResource/PhxResource_pch.h"
#include "PakFile.h"

using namespace phx;

PakFile::PakFile(std::filesystem::path const& path, FileHandle handle, std::shared_ptr<IResourceFileSystem> fs)
	: m_filePath(path)
	, m_cachedFilename(m_filePath.generic_string())
	, m_fileHandle(handle)
	, m_fs(std::move(fs))
	, m_status(Status_UnLoaded)
{

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

const PakFileFormat::AssetEntry* phx::PakFile::FindEntryByHash(phx::StringHash filename)
{
	size_t left = 0;
	size_t right = m_header.NumEntries - 1;

	while (left <= right)
	{
		size_t mid = left + (right - left) / 2;
		const PakFileFormat::AssetEntry& entry = m_assetEntries.Get()[mid];

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

RefCountPtr<PakFile> PakFileHandler::Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs) const
{
	FileHandle handle = fs->Open(path);
	if (!handle.IsValid())
		return nullptr;

	RefCountPtr<PakFile> pakFile = RefCountPtr<PakFile>::Create(new PakFile(path, handle, fs));

	// Begin Loading
	fs->EnqueueRead({
		.Handle = handle,
		.Offset = 0,
		.Size = sizeof(PakFileFormat::Header),
		.Dest = &pakFile->m_header
	});

	pakFile->m_status = PakFile::Status_LoadingMetdata;

	fs->SubmitRequests(
		[=] {
			const size_t fileSize = fs->GetFileSize(handle);
			const uint32_t sizeOfEntries = static_cast<uint32_t>(pakFile->m_header.NumEntries * sizeof(PakFileFormat::AssetEntry));
			pakFile->m_assetEntries = fs->EnqueueReadRegion<PakFileFormat::AssetEntry>(handle, sizeof(PakFileFormat::Header), sizeOfEntries);

			const uint32_t sizeOfStringEntries = static_cast<uint32_t>(pakFile->m_header.NumStrings * sizeof(PakFileFormat::StringEntry));
			const size_t stringTableOffset = sizeof(PakFileFormat::Header) + sizeOfEntries;
			pakFile->m_assetStringEntriesData = fs->EnqueueReadRegion<PakFileFormat::StringEntry>(handle, stringTableOffset, sizeOfStringEntries);
			pakFile->m_assetStringHeap = fs->EnqueueReadRegion<char>(handle, fileSize - pakFile->m_header.StringHeapSize, pakFile->m_header.StringHeapSize);

			fs->SubmitRequests([=]{
				pakFile->m_status = PakFile::Status_Loaded;
			});
		});

	return pakFile;
}