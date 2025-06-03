#pragma once

#include <PhxCore/Memory/MemorySystem.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>

#include <PhxResource/IResourceFileHandler.h>

namespace phx
{
	struct Resource;

	template<typename T>
	concept ResourceType = std::is_base_of_v<phx::Resource, T>;
	template<typename T>
	concept ResourceFileHandlerType = std::is_base_of_v<phx::IResourceFileHandler, T>;

	class ResourceSystem
	{
	public:
		inline static ResourceSystem* Ptr = nullptr;

		void Initialize(IFileSystem* fs);
		void Shutdown();

		RefCountPtr<Resource> Get(const char* path);

		template<ResourceType TResource>
		RefCountPtr<TResource> GetTyped(const char* path)
		{;
			return Get(path).As<TResource>();
		}

		template<ResourceFileHandlerType THandler>
		void RegisterFileHanlder()
		{
			constexpr const char* ext = ResourceFileExtension<THandler>::value;

#if false
			IAllocator& mainHeap = Memory::GetMainHeap();
			THandler* handler = phx_new<THandler>(mainHeap);
#endif
			m_resourceHandlers[StringHash(ext).ToHash()] = std::make_unique<THandler>();
		}

	private:
		IFileSystem* m_fs;
		std::mutex m_cacheMutex;
		std::unordered_map<Hash32, RefCountPtr<Resource>> m_cache;
		std::unordered_map<Hash32, std::unique_ptr<IResourceFileHandler>> m_resourceHandlers;
	};
}