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


	class ITexture : TypedAsset<AssetType::Texture>
	{
	public:
		virtual ~ITexture() = default;
	};

	class IMesh : TypedAsset<AssetType::Mesh>
	{
	public:
		virtual ~IMesh() = default;
	};

	class IMaterial : TypedAsset<AssetType::Material>
	{
	public:
		virtual ~IMaterial() = default;
	};

	class IAssetLoader
	{
	public:
		virtual ~IAssetLoader() = default;

		virtual bool Load(std::filesystem::path path) = 0;
	};


	namespace AssetLoaderFactory
	{
		std::unique_ptr<IAssetLoader> CreateMeshLoader();
		std::unique_ptr<IAssetLoader> CreateMaterialLoader();
		std::unique_ptr<IAssetLoader> CreateTextureLoader();
	}
}

