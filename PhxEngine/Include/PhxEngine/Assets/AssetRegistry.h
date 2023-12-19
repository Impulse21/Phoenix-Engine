#pragma once

#include <PhxEngine/Assets/Assets.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Log.h>

#include <mutex>
#include <memory>
#include <unordered_map>

namespace PhxEngine::Assets
{
	class AssetsRegistry
	{
	public:
		AssetsRegistry& GetInstance();

		void Initialize(std::unique_ptr<Core::IFileSystem>&& fileSystem);

		template<typename T>
		TypedAssetRef<T> Load(std::string const& filePath)
		{
			if (!this->m_fileSystem->FileExists(filePath))
			{
				PHX_LOG_CORE_ERROR("File doesn't exist on disk; %s", filePath.c_str());
				return nullptr;
			}

			// Check if the resource is already loaded
			const std::string name = Core::FileSystem::GetFileNameWithoutExt(filePath);
			auto cachedResource = this->GetByName<T>(name);
			if (cachedResource)
			{
				return cachedResource;
			}
		}
		
		template<typename T>
		std::shared_ptr<T> GetByName(std::string const& name)
		{
			std::scoped_lock _(this->m_registryMutex);
			auto itr = this->m_assets.find(name);
			
			if (itr != this->m_assets.end())
			{
				return std::static_pointer_cast<T>(itr->second);
			}

			return nullptr;
		}
		
	private:
		std::mutex m_registryMutex;
		std::unordered_map<std::string, AssetRef> m_assets;
		std::unique_ptr<Core::IFileSystem> m_fileSystem;

	};
}