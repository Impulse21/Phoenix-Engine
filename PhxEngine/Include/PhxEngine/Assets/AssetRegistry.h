#pragma once

#include <PhxEngine/Assets/Assets.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Log.h>

#include <mutex>
#include <memory>
#include <unordered_map>

namespace PhxEngine::Assets
{

	struct AssetGroup
	{
		size_t MemoryBudget = 0;
		size_t MemoryUsage = 0;

		std::unordered_map<Core::StringHash, std::shared_ptr<Asset>> Assets;
	};

	class AssetsRegistry : public Core::Object
	{
		PHX_OBJECT(AssetsRegistry, Object);

	public:
		AssetsRegistry(std::shared_ptr<Core::IFileSystem> fileSystem)
			: m_fileSystem(fileSystem) {}

		~AssetsRegistry();

	public:
		bool AddAssetDir(std::string const& pathName);
		bool AddManualAsset(Asset* asset);
		void RemoveAssetDir(std::string const& pathName);

		void ReleaseAsset(Core::StringHash type, std::string const& name, bool force = false);
		void ReleaseAssets(Core::StringHash type, bool force = false);

		void ReleaseAssets(std::string const& partialName, bool force = false);
		void ReleaseAllAssets(bool force = false);

		Asset* GetAsset(Core::StringHash type, std::string const& name, bool sendEventOnFailure = true);

		template<typename T>
		T* GetAsset(std::string const& name, bool sendEventOnFailure = true)
		{
			if (!Core::Threading::IsMainThread())
			{
				PHX_LOG_CORE_ERROR("Attempting to load Asset from out side of the main thread %s", name.c_str());
				return nullptr;
			}

			Core::StringHash nameHash(name);
			const std::shared_ptr<Asset>& existing = this->FindAsset(T::GetTypeNameStatic(), nameHash);
			if (existing)
			{
				return existing.get();
			}

			// Construct the resource
			if (!T::GetTypeInfoStatic()->IsTypeOf<Asset>());
			{
				PHX_LOG_CORE_ERROR("Could not load unknown asset type");
				return nullptr;
			}

			return nullptr;
		}

		std::shared_ptr<Asset> GetTempAsset(Core::StringHash type, std::string const& name, bool sendEventOnFailure = true);
		bool BackgroundLoadAsset(Core::StringHash type, std::string const& name, bool sendEventOnFailure = true, Asset* caller = nullptr);

		size_t GetNumBackgroundLoadAssets() const;
		void GetAssets(Core::Span<Asset*> result, Core::StringHash type) const;
		Asset* GetExistingAsset(Core::StringHash type, std::string const& name);

		template<class T> std::shared_ptr<T> GetTempAsset(std::string const& name, bool sendEventOnFailure = true)
		{
			return std::static_pointer_cast<T>(T::GetTypeStatic(), name, sendEventOnFailure);
		}

		template<class T> void ReleaseAsset(std::string const& name, bool force = false)
		{
			return this->ReleaseAsset(T::GetTypeStatic(), name, force);
		}

		template<class T> T* GetExistingAsset(Core::StringHash type, std::string const& name)
		{
			// return static_cast<T*>(this->GetExistingAsset(T::GetTypeStatic(), name);
			return nullptr
		}
		template<class T> T* GetAsset(std::string const& name, bool sendEventOnFailure)
		{
			return static_cast<T*>(this->GetAsset(T::GetTypeStatic(), name, sendEventOnFailure));
		}
		template<class T> bool BackgroundLoadAsset(std::string const& name, bool sendEventOnFailure = true, Asset* caller = nullptr)
		{
			return this->BackgroundLoadAsset(T::GetTypeStatic(), name, sendEventOnFailure, caller);
		}

		template<class T> void ReleaseAssets(Core::StringHash type, bool force = false)
		{
			return this->ReleaseAssets(T::GetTypeStatic(), force);
		}

	private:
		void UpdateResourceGroup(Core::StringHash type);
		const std::shared_ptr<Asset>& FindAsset(Core::StringHash type, Core::StringHash nameHash);
		const std::shared_ptr<Asset>& FindAsset(Core::StringHash nameHash);

	private:
		std::mutex m_registryMutex;
		std::unordered_map<Core::StringHash, AssetGroup> m_assetGroups;
		Core::FlexArray<std::string> m_AssetDirectories;

		// TODO: Async loader
		bool m_returnFailedAssets;
		/// Search priority flag.
		bool m_searchPackagesFirst;
		/// How many milliseconds maximum per frame to spend on finishing background loaded Assets.
		int32_t m_finishBackgroundAssetsMs;

		std::shared_ptr<Core::IFileSystem> m_fileSystem;
	};
}