#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Handle.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxEngine/IO/IoQueue.h>
#include <PhxEngine/StreamingDefintions.h>

#include <PhxRhi/PhxRhi_Types.h>

#include "IResourceLoader.h"
#include "ResourcePtr.h"
#include "ResourceTypes.h"
#include "ResourceStore.h"

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

        static void Initialize();
        static void Shutdown();

        // --- 1. Explicit Registration ---
        template<typename T>
        static void RegisterType(uint16_t capacity, bool(*collect_transitions)(GenericHandle, SpanMutable<rhi::GpuBarrier>, size_t&) = nullptr);

        template<ResourceLoaderType T>
        static void RegisterLoader(const char* ext)
        {
            ms_loaders[ext] = std::move(std::make_unique<T>());
        }

        // --- 2. Public API ---
        template<typename T> 
        static Handle<T> Load(const char* path);

        template<typename T> 
        static auto* Get(Handle<T> h) { return ResourceStore<T>::GetHot(h); }

        template<typename T>
        static bool IsLoaded(Handle<T> h)
        {
            auto* hot = ResourceStore<T>::GetHot(h);
            return hot && hot->state == ResourceState::Loaded;
		}

        static bool IsLoaded(GenericHandle h);
		static bool IsErrorState(GenericHandle h);
        static void SetState(GenericHandle h, ResourceState state);

        static void IncRef(GenericHandle h);
        static void DecRef(GenericHandle h);

		static void PushToGpuTransitionQueue(GenericHandle handle);
		static void PopPendingGpuTransitions(std::vector<GenericHandle>& generic_handles);

        static bool CollectPendingGpuTransitions(GenericHandle handle, SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index);

    private:
        static void RegisterStoreInterface(
            uint16_t id,
            void(*inc)(GenericHandle),
            void(*dec)(GenericHandle),
            bool(*is_loaded)(GenericHandle),
            bool (*is_error_state)(GenericHandle),
            void(*set_state)(GenericHandle, ResourceState),
            bool(*collect_transitions)(GenericHandle, SpanMutable<rhi::GpuBarrier>, size_t&));

        inline static std::unique_ptr<AsyncLoader> ms_async_loader;
        inline static std::unordered_map<std::string, GenericHandle> ms_path_cache;
        inline static std::shared_mutex ms_cache_mutex;
        inline static std::unordered_map<std::string, std::unique_ptr<IResourceLoader>> ms_loaders;
		inline static std::vector<GenericHandle> ms_gpu_transition_queue;
        inline static std::mutex ms_gpu_queue_mutex;
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
    void ResourceManager::RegisterType(uint16_t capacity, bool(*collect_transitions_fn)(GenericHandle, SpanMutable<rhi::GpuBarrier>, size_t&))
    {
        ResourceStore<T>::Initialize(capacity);

        RegisterStoreInterface(
            ResourceTypeId<T>::Get(),
            &ResourceStore<T>::IncRefGeneric,
            &ResourceStore<T>::DecRefGeneric,
			&ResourceStore<T>::IsLoadedGeneric,
            &ResourceStore<T>::IsErrorStateGeneric,
			&ResourceStore<T>::SetStateGeneric,
            collect_transitions_fn
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
        
        ms_async_loader->QueueRequest({
            .handle = GenericHandle::From(resource_handle),
            .virtual_path = virtual_file_path,
            .loader_interface = loader,
        });

        {
            std::scoped_lock lock(ms_cache_mutex);
            ms_path_cache[virtual_file_path] = 
                GenericHandle::From(resource_handle);
        }

        return resource_handle;
    }
}