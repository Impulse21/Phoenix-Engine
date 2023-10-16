#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <filesystem>

namespace PhxEngine::Assets
{
	enum class AssetType
	{
		Texture,
		Material,
		Mesh,
	};

	class IAsset
	{
	public:
		virtual ~IAsset() = default;
	};

	template<AssetType _AssetType>
	class TypedAsset : public IAsset
	{
	public:
		static AssetType GetStaticType() { return _AssetType; }

		AssetType GetType() { return _AssetType; }
	};

	class ILoadedTexture : TypedAsset<AssetType::Texture>
	{
	public:
		virtual ~ILoadedTexture() = default;
	};
	using LoadedTextureHandle = std::shared_ptr<ILoadedTexture>;

	class IMesh : TypedAsset<AssetType::Mesh>
	{
	public:
		virtual ~IMesh() = default;
	};
	using MeshHandle = std::shared_ptr<IMesh>;

	struct MaterialDesc
	{

	};

	class IMaterial : TypedAsset<AssetType::Material>
	{
	public:
		virtual ~IMaterial() = default;
	};
	using MaterialHandle = std::shared_ptr<IMaterial>;

	class IAssetLoader
	{
	public:
		virtual ~IAssetLoader() = default;

		virtual bool Load(std::filesystem::path path) = 0;
	};


	namespace AssetFactory
	{
		std::unique_ptr<IMaterial> CreateMaterialAsset(MaterialDesc desc);
		std::unique_ptr<IAssetLoader> CreateMaterialLoader();
		std::unique_ptr<IAssetLoader> CreateTextureLoader();
	}
}

