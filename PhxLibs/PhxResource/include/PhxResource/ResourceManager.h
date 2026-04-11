#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Handle.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/VirtualFileSystem.h>

#include <PhxResource/IO/IoQueue.h>
#include <PhxResource/IO/StreamingDefintions.h>

#include <PhxRhi/PhxRhi_Types.h>

#include "Resource.h"

#include "IResourceLoader.h"
#include "ResourceTypes.h"

#include "AsyncLoader.h"

#include <vector>
#include <string>
#include <shared_mutex>

namespace phx
{
    template<typename T>
    concept ResourceLoaderType = std::is_base_of_v<phx::IResourceLoader, T>;

    class ResourceManager
    {
    public:
        ResourceManager() = delete;

        static void Initialize(ThreadPoolHandle streaming_thread_pool_handle);
        static void Shutdown();


        template<ResourceLoaderType T>
        static void RegisterLoader(const char* ext)
        {
            ms_loaders[ext] = std::move(std::make_unique<T>());
        }
        
        template<ResourceLoaderType T>
        static void RegisterLoader()
        {
            constexpr const char* ext = phx::ResourceTraits<T>::Extension;
            ms_loaders[ext] = std::move(std::make_unique<T>());
        }

        template<typename T> 
        static RefCountPtr<T> Load(const char* path);

        template<typename T> 
        static RefCountPtr<T> Get(const char* path);

		static void PushToGpuTransitionQueue(RefCountPtr<Resource> resource);
		static void PopPendingGpuTransitions(std::vector<RefCountPtr<Resource>>& generic_handles);

    private:
        inline static std::unique_ptr<AsyncLoader> ms_async_loader;
        inline static std::unordered_map<std::string, RefCountPtr<Resource>> ms_path_cache;
        inline static std::shared_mutex ms_cache_mutex;
        inline static std::unordered_map<std::string, std::unique_ptr<IResourceLoader>> ms_loaders;
		inline static std::vector<RefCountPtr<Resource>> ms_gpu_transition_queue;
        inline static std::mutex ms_gpu_queue_mutex;
    };

}

namespace phx
{
    template <typename T>
    RefCountPtr<T> ResourceManager::Get(const char *virtual_file_path)
    {
        std::shared_lock lock(ms_cache_mutex);
        if (auto it = ms_path_cache.find(virtual_file_path); it != ms_path_cache.end())
        {
            return it->second.As<T>();
        }

        return nullptr;
    }

    template<typename T>
    RefCountPtr<T> ResourceManager::Load(const char* virtual_file_path)
    {
        // -- check cache ---
        {
            std::shared_lock lock(ms_cache_mutex);
            if (auto it = ms_path_cache.find(virtual_file_path); it != ms_path_cache.end())
            {
                return it->second.As<T>();
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

            return nullptr;
        }

        std::scoped_lock lock(ms_cache_mutex);
        if (auto it = ms_path_cache.find(virtual_file_path); it != ms_path_cache.end())
        {
            return it->second.As<T>();
        }

        auto resource = RefCountPtr<T>::Create();

        PHX_CORE_INFO(
            "Loading Resource '{0}' from disk",
            virtual_file_path);
        
        ms_async_loader->QueueRequest({
            .handle = resource,
            .virtual_path = virtual_file_path,
            .loader_interface = loader,
        });
        
        ms_path_cache[virtual_file_path] = resource;
        return resource;
    }
}