#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Scene/AssetStore.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Scene;

Core::Handle<Scene::StaticMeshAsset> AssetStore::CreateMesh(std::string_view filename, bool isAsync)
{
	return {};
}

Core::Handle<Scene::TextureAsset> AssetStore::CreateTexture(std::string_view filename, bool isAsync)
{
	return {};
}

MaterialAssetHandle AssetStore::CreateMaterial()
{
	return {};
}

MaterialAsset* AssetStore::GetStaticMeshAsset(Core::Handle<Scene::StaticMeshAsset> handle)
{
	return nullptr;
}

MaterialAsset* AssetStore::GetMaterialAsset(MaterialAssetHandle handle)
{

	return nullptr;

}