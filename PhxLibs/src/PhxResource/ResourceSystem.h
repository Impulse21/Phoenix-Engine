#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Pool.h>
#include <PhxCore/RefCountPtr.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxResource/IResourceFileHandler.h>
#include <PhxResource/Resource.h>

#include <PhxEngine/StreamingDefintions.h>
#include <PhxEngine/IO/IoQueue.h>

namespace phx
{
	template<typename T>
	concept ResourceType = std::is_base_of_v<phx::Resource, T>;
	template<typename T>
	concept ResourceFileHandlerType = std::is_base_of_v<phx::ResourceFileHandler, T>;

	namespace NEW
	{
		struct Resource
		{
			std::atomic_uint16_t ref_count;
		};
	}

	class ResourceSystem
	{
	public:
		inline static ResourceSystem* Ptr = nullptr;

	public:
		void Initialize(IVirtualFileSystem* fs);
		void Shutdown();

		// -- new --
	public:
		template<typename T>
		Handle<T> Load(const char* virtual_path);

		template<typename T>
		T* Get(Handle<T> handle);

		template<typename T>
		void IncRef(Handle<T> handle)
		{
			auto* cold = GetPool<T>().GetCold(handle);
			if (cold) 
				cold->ref_count.fetch_add(1, std::memory_order_relaxed);
		}

		template<typename T>
		void DecRef(Handle<T> handle)
		{
			auto* cold = GetPool<T>().GetCold(handle);
			if (cold) 
				cold->ref_count.fetch_sub(1, std::memory_order_relaxed);
		}

		// --- The Magic Pool Accessor ---
		// Public so Pre-Cache can iterate it.
		// Defined below.
		template<typename T>
		PagedPool<T,T, NEW::Resource>& GetPool();
		
	public:
		RefCountPtr<Resource> Get(const char* path);

		template<ResourceType T>
		std::pair<RefCountPtr<T>, bool> FindOrCreatePlaceholder(const std::string& virtual_path);

		template<ResourceFileHandlerType THandler>
		void RegisterFileHanlder()
		{
			constexpr const char* ext = ResourceFileExtension<THandler>::value;
			m_resourceHandlers[StringHash(ext).ToHash()] = std::make_unique<THandler>();
		}

	private:
		IVirtualFileSystem* m_vfs;
		std::mutex m_cacheMutex;
		std::unordered_map<Hash32, RefCountPtr<Resource>> m_cache;
		std::unordered_map<Hash32, std::unique_ptr<ResourceFileHandler>> m_resourceHandlers;
	};

	inline RefCountPtr<Resource> ResourceSystem::Get(const char* virtual_file_path)
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
			StringHash extId(ext);
			auto handler_itr = m_resourceHandlers.find(extId);

			if (handler_itr == m_resourceHandlers.end())
			{
				PHX_CORE_ERROR("Resource Type mismatch '{0}'", ext.c_str());
				return nullptr;
			}

			handler_to_use = handler_itr->second.get();
			placeholder = handler_to_use->CreatePlaceholder();
			placeholder->state = Resource::State::Loading;
			m_cache[filenameHash] = placeholder;
		}

		PHX_CORE_INFO(
			"Loading Resource '{0}' from disk",
			virtual_file_path);

		Result<AsyncResourceDescriptor> resource_descriptor = m_vfs->GetResourceDescriptorForAsync(virtual_file_path);
		if (resource_descriptor.HasError())
		{
			PHX_CORE_INFO(
				"Failed to load resource '{0}'. Unable to retrieve resource descriptor",
				virtual_file_path);

			placeholder->state = Resource::State::Error;
			return placeholder;
		}

		handler_to_use->LoadAsync(IIoQueue::Ptr, placeholder, resource_descriptor.GetValue());

		return placeholder;
	}

	template<ResourceType T>
	inline std::pair<RefCountPtr<T>, bool> ResourceSystem::FindOrCreatePlaceholder(const std::string& virtual_path)
	{
		StringHash filenameHash(virtual_path);

		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return std::make_pair(itr->second.As<T>(), false);

		auto placeholder = RefCountPtr<T>::Create();
		m_cache[filenameHash] = placeholder;
		return std::make_pair(placeholder, true);
	}
}