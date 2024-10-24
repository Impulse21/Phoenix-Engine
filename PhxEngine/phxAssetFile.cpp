#include "pch.h"
#include "phxAssetFile.h"
#include "phxVFS.h"

bool phx::assets::LoadBinaryFile(IFileSystem* fs, const char* path, AssetFile& assetFile)
{
    return false;
}

bool phx::assets::SaveBinaryFile(IFileSystem* fs, const char* path, AssetFile const& assetFile)
{
#if false
	std::stringstream ss;

	ss << assetFile.ID;
	ss << assetFile.Version;
	ss << (uint32_t)assetFile.json.size();
	ss << (uint32_t)assetFile.json;
	ss << (uint32_t)assetFile.bloblength.size();
	ss << (uint32_t)assetFile.binaryBlob;

	fs->WriteFile(path, fileData);

#endif
	return true;
}
