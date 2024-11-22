#include "pch.h"
#include "phxAssetFile.h"
#include "phxVFS.h"
#include "phxBinaryBuilder.h"

bool phx::assets::LoadBinaryFile(IFileSystem* fs, const char* path, AssetFile& assetFile)
{
	std::unique_ptr<IBlob> fileData = fs->ReadFile(path);
	if (!fileData)
	{
		return false;
	}

	// parse the Blob;
	size_t offset = 0;
	std::memcpy(&assetFile.Header, fileData->Data(), sizeof(AssetFile::Header));
	size_t size = (size_t)*reinterpret_cast<uint32_t*>((uint8_t*)fileData->Data() + sizeof(AssetFile::Header));
	assetFile.JsonMetadata.resize(size);
	std::memcpy(assetFile.JsonMetadata.data(), (char*)fileData->Data() + sizeof(AssetFile::Header) + 1, assetFile.JsonMetadata.size());

	std::memcpy(assetFile.JsonMetadata.data(), (char*)fileData->Data() + sizeof(AssetFile::Header) + 1, assetFile.JsonMetadata.size());
    return true;
}

bool phx::assets::SaveBinaryFile(IFileSystem* fs, const char* path, AssetFile const& assetFile)
{
#if true
	BinaryBuilder builder;

	BinaryBuilder::OffsetHandle headerHandle = builder.Reserve<AssetFile::HeaderDef>();
	BinaryBuilder::OffsetHandle metadataHandle = builder.Reserve(assetFile.JsonMetadata.size() + 1);
	BinaryBuilder::OffsetHandle binaryHandle = builder.Reserve(assetFile.BinaryBlob.size() + 1);

	builder.Commit();

	auto* headerDst = builder.Place<AssetFile::HeaderDef>(headerHandle);
	*headerDst = assetFile.Header;

	auto* metadataDst = builder.Place<char>(metadataHandle);
	*metadataDst = assetFile.JsonMetadata.size();
	std::memcpy(metadataDst + 1, assetFile.JsonMetadata.data(), assetFile.JsonMetadata.size());

	auto* binaryBlobDst = builder.Place<char>(binaryHandle);
	*binaryBlobDst = assetFile.BinaryBlob.size();
	std::memcpy(binaryBlobDst + 1, assetFile.BinaryBlob.data(), assetFile.BinaryBlob.size());

	Span<uint8_t> memory = builder.GetMemory();
	return fs->WriteFile(path, { reinterpret_cast<const char*>(memory.begin()), memory.Size()});

#else 
	std::stringstream ss;

	ss.write(reinterpret_cast<const char*>(&assetFile.Header), sizeof(AssetFile::Header));
	ss.write(assetFile.JsonMetadata.data(), assetFile.JsonMetadata.size());
	ss.write(assetFile.BinaryBlob.data(), assetFile.BinaryBlob.size());

	std::string output = ss.str();
	return fs->WriteFile(path, { output.data(), output.size() });

#endif
}
