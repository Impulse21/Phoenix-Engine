#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Handle.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxEngine/IO/IoQueue.h>
#include <PhxEngine/StreamingDefintions.h>

#include "IResourceLoader.h"
#include "ResourcePtr.h"
#include "ResourceTypes.h"
#include "ResourceStore.h"

#include <vector>
#include <string>
#include <shared_mutex>

namespace phx
{

#if false

    template<typename T>
    concept ResourceFileHandlerType = std::is_base_of_v<phx::ResourceFileHandler, T>;
#endif

    class ResourceManager
    {
    public:
        ResourceManager() = delete;

        static void Initialize();
        static void Shutdown();

        // --- 1. Explicit Registration ---
        template<typename T>
        static void RegisterType(uint16_t capacity);

        static void RegisterLoader(const char* ext, std::unique_ptr<IResourceLoader> loader)
        {
            ms_loaders[ext] = std::move(loader);
        }

#if false

        template<ResourceFileHandlerType THandler>
        void RegisterFileHanlder()
        {
            constexpr const char* ext = ResourceFileExtension<THandler>::value;
            m_resourceHandlers[StringHash(ext).ToHash()] = std::make_unique<THandler>();
        }
#endif

        // --- 2. Public API ---
        template<typename T> static Handle<T> Load(const char* path);
        template<typename T> static auto* Get(Handle<T> h) { return ResourceStore<T>::GetHot(h); }

        static void IncRef(GenericHandle h);
        static void DecRef(GenericHandle h);

    private:
        static void RegisterStoreInterface(uint16_t id, void(*inc)(GenericHandle), void(*dec)(GenericHandle));

        inline static std::unordered_map<std::string, GenericHandle> ms_path_cache;
        inline static std::shared_mutex ms_cache_mutex;
        inline static std::unordered_map<std::string, std::unique_ptr<IResourceLoader>> ms_loaders;
    };

}

namespace phx
{
    template<typename T>
    inline T* ResourcePtr<T>::operator->() const
    {
        // Now ResourceManager is fully defined, so this works!
        T* ptr = ResourceManager::Get(m_handle);
        return ptr ? ptr : nullptr;
    }

    template<typename T>
    inline T* ResourcePtr<T>::Get() const
    {
        return ResourceManager::Get(m_handle);
    }
}

namespace phx
{
    template<typename T>
    void ResourceManager::RegisterType(uint16_t capacity)
    {
        ResourceStore<T>::Initialize(capacity);

        RegisterStoreInterface(
            ResourceTypeId<T>::Get(),
            &ResourceStore<T>::IncRefGeneric,
            &ResourceStore<T>::DecRefGeneric
        );
    }

    template<typename T>
    Handle<T> ResourceManager::Load(const char* virtual_file_path)
    {
        // -- check cache ---
        {
            std::shared_lock lock(ms_cache_mutex);
            if (auto it = ms_path_cache.find(virtual_file_path); it != ms_path_cache.end())
            {
                return it->second.To<T>();
            }
        }

        // -- grab loader ---
        std::string ext = phx::GetFileExt(virtual_file_path);
        auto* loader = ms_loaders[ext].get();
        if (!loader)
        {
            PHX_CORE_ERROR("Resource Type mismatch '{0}'", ext.c_str());
            return {};
        }

        // -- grab streaming info ---
        auto* vfs = IVirtualFileSystem::Ptr;
        Result<AsyncResourceDescriptor> resource_descriptor =
            vfs->GetResourceDescriptorForAsync(virtual_file_path);

        if (resource_descriptor.HasError())
        {
            PHX_CORE_INFO(
                "Failed to load resource '{0}'. Unable to retrieve resource descriptor",
                virtual_file_path);

            return {};
        }

        Handle<T> resource_handle = ResourceStore<T>::Allocate();

        PHX_CORE_INFO(
            "Loading Resource '{0}' from disk",
            virtual_file_path);
        
        auto io_queue = IoQueue::Ptr;
        StreamingRequest req;
        loader->PrepareRequest(req, GenericHandle::From(resource_handle), io_queue, resource_descriptor.GetValue());
        io_queue->Submit(std::move(req));

        {
            std::scoped_lock lock(ms_cache_mutex);
            ms_path_cache[virtual_file_path] = 
                GenericHandle::From(resource_handle);
        }

        return resource_handle;
    }
}