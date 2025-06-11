#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#include <PhxData/IAssetImporter.h>
#include <PhxData/Asset.h>

#include <mutex>
#include <unordered_map>

#include <memory>

namespace phx::data
{
	class IVirtualFileSystem;
	class IAsyncIOSystem;

	template<typename T>
	concept AssetType = std::is_base_of_v<phx::data::Asset, T>;
	template<typename T>
	concept AssetImportHandlerType = std::is_base_of_v<IAssetImporter, T>;

	class AssetManager final
	{
	public:
		inline static AssetManager* Ptr = nullptr;

	public:
		void Initialize(IVirtualFileSystem* fs, IAsyncIOSystem* loader);
		void Shutdown();

		RefCountPtr<Asset> Get(const char* virtual_file_path);

		template<AssetType TAsset>
		RefCountPtr<TAsset> GetTyped(const char* virtual_file_path)
		{
			return Get(virtual_file_path).As<TAsset>();
		}

		template<AssetImportHandlerType TImporter>
		void RegisterImporter()
		{
			constexpr const char* ext = AssetImporterFileExtension<TImporter>::value;
			RegisterImporter(ext, std::make_unique<TImporter>());
		}

	private:
		IVirtualFileSystem* m_vfs;
		IAsyncIOSystem* m_loader;
		std::mutex m_cacheMutex;
		std::unordered_map<StringHash, RefCountPtr<Asset>> m_cache;
		std::unordered_map<StringHash, std::unique_ptr<IAssetImporter>> m_assetImporters;
	};
}

