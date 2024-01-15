#pragma once
#if false
#include <PhxEngine/Assets/AssetFile.h>
#include "ModelInfoBuilder.h"
#include <PhxEngine/Core/VirtualFileSystem.h>

class AssetPacker
{
public:
	virtual ~AssetPacker() = default;

	static PhxEngine::Assets::AssetFile Pack(MeshInfo& meshInfo);
	static PhxEngine::Assets::AssetFile Pack(MaterialInfo& mtlInfo);

	static bool SaveBinary(IFileSystem* fs, const char* filename, PhxEngine::Assets::AssetFile const& assetFile);
protected:
	AssetPacker() = default;

protected:
};
#endif