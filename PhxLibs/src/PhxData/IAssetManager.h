#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxData/IAssetImporter.h>
#include <PhxData/Asset.h>

#include <memory>

namespace phx::data
{

	template<typename T>
	struct AssetImporterFileExtension;

	template<typename T>
	struct AssetImporterFileHandlerId;

	namespace data
	{
		class IVirtualFileSystem;
		class IAsyncIOSystem;
	}

	template<typename T>
	concept AssetType = std::is_base_of_v<phx::data::Asset, T>;
	template<typename T>
	concept AssetImportHandlerType = std::is_base_of_v<IAssetImporter, T>;

	class IAssetManager
	{
	public:
		virtual RefCountPtr<Asset> Get(const char* path) = 0;
		virtual void RegisterImporter(const char* extension, std::unique_ptr<IAssetImporter> importer) = 0;

		template<AssetType TAsset>
		RefCountPtr<TAsset> GetTyped(const char* path)
		{
			return Get(path).As<TAsset>();
		}

		template<AssetImportHandlerType TImporter>
		void RegisterImporter()
		{
			constexpr const char* ext = AssetImporterFileExtension<TImporter>::value;
			RegisterImporter(ext, std::make_unique<TImporter>());
		}
		virtual ~IAssetManager() = default;
	};
}

