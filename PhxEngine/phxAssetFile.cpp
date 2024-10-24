#include "pch.h"
#include "phxAssetFile.h"
#include "phxVFS.h"

bool phx::assets::LoadBinaryFile(IFileSystem* fs, const char* path, AssetFile& assetFile)
{
    return false;
}

bool phx::assets::SaveBinaryFile(IFileSystem* fs, const char* path, AssetFile const& assetFile)
{

	std::stringstream ss;

	ss << assetFile.ID;
	ss << assetFile.Version;
	ss << (uint32_t)assetFile.json.size();

	fs->WriteFile(path, fileData);
	
	outfile.write(file.type, 4);
	uint32_t version = file.version;
	//version
	outfile.write((const char*)&version, sizeof(uint32_t));

	//json length
	uint32_t length = file.json.size();
	outfile.write((const char*)&length, sizeof(uint32_t));

	//blob length
	uint32_t bloblength = file.binaryBlob.size();
	outfile.write((const char*)&bloblength, sizeof(uint32_t));

	//json stream
	outfile.write(file.json.data(), length);
	//blob data
	outfile.write(file.binaryBlob.data(), file.binaryBlob.size());

	outfile.close();

	return true;
}
