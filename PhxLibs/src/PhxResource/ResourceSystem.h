#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>

#include <PhxResource/IResourceFileHandler.h>

namespace phx
{
	struct Resource;

	template<typename T>
	concept ResourceType = std::is_base_of_v<phx::Resource, T>;
	template<typename T>
	concept ResourceFileHandlerType = std::is_base_of_v<phx::ResourceFileHandler, T>;

	namespace data
	{
		class IVirtualFileSystem;
		class IAsyncIOSystem;
	}

	class ResourceSystem
	{
	public:
		inline static ResourceSystem* Ptr = nullptr;

		void Initialize(data::IVirtualFileSystem* fs, data::IAsyncIOSystem* loader);
		void Shutdown();

		RefCountPtr<Resource> Get(const char* path);

		template<ResourceType T>
		std::pair<RefCountPtr<T>, bool> FindOrCreatePlaceholder(const std::string& virtual_path)
		{
			StringHash filenameHash(virtual_path);
			{
				std::scoped_lock _(m_cacheMutex);
				auto itr = m_cache.find(filenameHash);
				if (itr != m_cache.end())
					return std::make_pair(itr->second, false);
			}

			auto ptr = RefCountPtr<T>::Create(new T);
			RegisterSubResource(virtual_path, ptr);

			return std::make_pair(ptr, true);
		}

		void RegisterSubResource(const std::string& virtual_path, RefCountPtr<Resource> resource);

		template<ResourceType TResource>
		RefCountPtr<TResource> GetTyped(const char* path)
		{;
			return Get(path).As<TResource>();
		}

		template<ResourceFileHandlerType THandler>
		void RegisterFileHanlder()
		{
			constexpr const char* ext = ResourceFileExtension<THandler>::value;

			m_resourceHandlers[StringHash(ext).ToHash()] = std::make_unique<THandler>();
		}

	private:
		data::IVirtualFileSystem* m_vfs;
		data::IAsyncIOSystem* m_loader;
		std::mutex m_cacheMutex;
		std::unordered_map<Hash32, RefCountPtr<Resource>> m_cache;
		std::unordered_map<Hash32, std::unique_ptr<ResourceFileHandler>> m_resourceHandlers;
	};
}