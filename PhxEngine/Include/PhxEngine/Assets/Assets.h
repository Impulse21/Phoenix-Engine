#pragma once

#include <PhxEngine/Core/RefCountPtr.h>

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

	class IMesh : TypedAsset<AssetType::Texture>
	{
	public:
		virtual ~IMesh() = default;
	};

	class IMaterial : TypedAsset<AssetType::Texture>
	{
	public:
		virtual ~IMaterial() = default;
	};

}

