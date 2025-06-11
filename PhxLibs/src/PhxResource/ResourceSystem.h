#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>
#include <PhxCore/IO/FileUtils.h>

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

	public:
		void Initialize(data::IVirtualFileSystem* fs, data::IAsyncIOSystem* loader);
		void Shutdown();

		template<ResourceType TResource>
		RefCountPtr<TResource> Get(const char* path);

		template<ResourceType T>
		std::pair<RefCountPtr<T>, bool> FindOrCreatePlaceholder(const std::string& virtual_path);

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


	template<ResourceType TResource>
	inline RefCountPtr<TResource> ResourceSystem::Get(const char* virtual_file_path)
	{
		StringHash filenameHash(virtual_file_path);

		RefCountPtr<Resource> placeholder = nullptr;
		ResourceFileHandler* handler_to_use = nullptr;
		// -- Critical section ---
		{
			std::scoped_lock _(m_cacheMutex);
			auto itr = m_cache.find(filenameHash);
			if (itr != m_cache.end())
				return itr->second;

			std::string ext = phx::GetFileExt(virtual_file_path);
			auto handler_itr = m_resourceHandlers.find(StringHash(ext));

			if (handler_itr == m_resourceHandlers.end() || handler_itr->GetResourceTypeHash() == TResource::StaticTypeHash)
			{
				PHX_CORE_ERROR("Resource Type mismatch '{0}'", ext.c_str());
				return nullptr;
			}

			placeholder = RefCountPtr<TResource>::Create();
			placeholder->state = Resource::State::Loading;
			m_cache[path] = placeholder;
			handler_to_use = handler_itr->second.get();
		}

		PHX_CORE_INFO(
			"Loading Resource '{0}' from disk",
			virtual_file_path);

		LoadAsyncContext ctx =
		{
			.virtual_file_path = virtual_file_path,
			.resource = placeholder,
			.resource_system = this,
			.vfs = m_vfs,
			.loader = m_lodaer
		};
		handler_to_use->second->LoadAsync(ctx);

		return placeholder;
	}

	template<ResourceType T>
	inline std::pair<RefCountPtr<T>, bool> ResourceSystem::FindOrCreatePlaceholder(const std::string& virtual_path)
	{
		StringHash filenameHash(virtual_path);

		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return std::make_pair(itr->second, false);

		auto placeholder = RefCountPtr<T>::Create();
		m_cache[path] = placeholder;
		return std::make_pair(placeholder, true);
	}
}