#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/IO/FileUtils.h>

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

		template<AssetType TAsset>
		RefCountPtr<TAsset> Get(const char* virtual_file_path);

		template<AssetType TAsset>
		RefCountPtr<TAsset> RegisterPrecreatedAsset(const char* virtual_file_path);

		template<AssetImportHandlerType TImporter>
		void RegisterImporter();

	private:
		IVirtualFileSystem* m_vfs;
		IAsyncIOSystem* m_loader;
		std::mutex m_cacheMutex;
		std::unordered_map<StringHash, RefCountPtr<Asset>> m_cache;
		std::unordered_map<StringHash, std::unique_ptr<IAssetImporter>> m_assetImporters;
	};


	template<AssetType TAsset>
	RefCountPtr<TAsset> AssetManager::Get(const char* virtual_file_path)
	{
		StringHash filenameHash(virtual_file_path);
		RefCountPtr<TAsset> placeholder = nullptr;
		IAssetImporter* importer_to_use = nullptr;

		// -- Critical section ---
		{
			std::scoped_lock _(m_cacheMutex);
			auto itr = m_cache.find(filenameHash);
			if (itr != m_cache.end())
				return itr->second.As<TAsset>();

			std::string ext = phx::GetFileExt(virtual_file_path);
			auto handler_itr = m_assetImporters.find(StringHash(ext));

			if (handler_itr == m_assetImporters.end() || handler_itr->second->GetAssetTypeHash() == TAsset::StaticTypeHash())
			{
				PHX_CORE_ERROR("Asset Type mismatch '{0}'", ext.c_str());
				return nullptr;
			}

			placeholder = RefCountPtr<TAsset>::Create();
			placeholder->state = Asset::State::Loading;
			m_cache[filenameHash] = placeholder;
			importer_to_use = handler_itr->second.get();
		}

		PHX_CORE_INFO(
			"Importing asset '{0}' from disk",
			virtual_file_path);

		importer_to_use->ImportAsync(this, placeholder, virtual_file_path);
		return placeholder;
	}

	template<AssetType TAsset>
	inline RefCountPtr<TAsset> AssetManager::RegisterPrecreatedAsset(const char* virtual_file_path)
	{
		auto placeholder = RefCountPtr<TAsset>::Create();
		m_cache[virtual_file_path] = placeholder;

		return placeholder;
	}

	template<AssetImportHandlerType TImporter>
	inline void phx::data::AssetManager::RegisterImporter()
	{
		constexpr const char* ext = AssetImporterFileExtension<TImporter>::value;
		m_assetImporters[StringHash(ext).ToHash()] = std::make_unique<TImporter>();
	}
}

