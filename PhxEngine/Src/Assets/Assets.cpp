#include <PhxEngine/Assets/Assets.h>

using namespace PhxEngine::Assets;



template <typename T>
inline constexpr AssetType Asset::TypeToEnum() { return AssetType::Unknown; }

template<typename T>
constexpr void ValidAssetType() { static_assert(std::is_base_of<Asset, T>::value, "Provided type does not implement Asset"); }

// Explicit template instantiation
#define INSTANTIATE_TO_RESOURCE_TYPE(T, enumT) template<> AssetType Asset::TypeToEnum<T>() { ValidAssetType<T>(); return enumT; }

// To add a new resource to the engine, simply register it here
INSTANTIATE_TO_RESOURCE_TYPE(Material, AssetType::Material)
INSTANTIATE_TO_RESOURCE_TYPE(Mesh, AssetType::Mesh)

PhxEngine::Assets::Mesh::Mesh()
	: Asset(AssetType::Mesh)
{
}

PhxEngine::Assets::Mesh::Mesh(MeshMetadata const& metdata)
	: Asset(AssetType::Mesh)
{
}
