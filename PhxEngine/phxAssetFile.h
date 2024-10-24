#pragma once

#define MAKE_ID(a,b,c,d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))


namespace phx
{
	class IFileSystem;
}
namespace phx::assets
{
	constexpr uint32_t kCurrentFileVersion = 1;
	struct AssetFile
	{
		uint32_t ID;
		uint32_t Version = kCurrentFileVersion;
		std::string json;
		std::vector<char> BinaryBlob;
	};

	bool LoadBinaryFile(IFileSystem* fs, const char* path, AssetFile& loadAssetFile);
	bool SaveBinaryFile(IFileSystem* fs, const char* path, AssetFile const& loadAssetFile);
}

