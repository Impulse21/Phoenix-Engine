#pragma once

#include <rfl.hpp>
#include <rfl/yaml.hpp>

#include <PhxAsset/AssetPtr.h>

#include <PhxCore/UUID.h>
#include <PhxAsset/ReflectCppBindings.h>
#include <PhxCore/Reflect/Reflection.h>

#include <memory>

namespace phx::asset
{
    template<typename T>
    concept IsAsset = requires(T t) { t.header; };

    // -- This is a static class as I require private fields for cache lookups.
    class AssetDB final
    {
    public:
        static void Initialize(IVirtualFileSystem* vfs)
        {
            ms_vfs = vfs;
        }

        template <IsAsset TAsset>
        static asset::Ptr<TAsset> Get(std::string_view virtual_file_path);

    private:
        AssetDB() = default;

        template<IsAsset TAsset>
        static asset::Ptr<TAsset> Load(std::string_view virtual_path);

    private:
        struct AssetSlot
        {
            std::unique_ptr<void, void(*)(void*)>  asset;
            UUID asset_id;
        };

        inline static phx::IVirtualFileSystem* ms_vfs = nullptr;
        inline static std::unordered_map<std::string, AssetSlot> ms_cache;
        inline static std::mutex ms_cache_mutex;
    };

    template <IsAsset TAsset>
    inline asset::Ptr<TAsset> AssetDB::Get(std::string_view virtual_path)
    {
        const std::string key(virtual_path);

        // -- Cache Hit ---
        {
            std::scoped_lock _(ms_cache_mutex);

            auto it = ms_cache.find(key);
            if (it != ms_cache.end())
                return asset::Ptr(static_cast<TAsset*>(it->second.asset.get()));
        }

        // -- Load ---
        return AssetDB::Load<TAsset>(virtual_path);
    }
    
    template<IsAsset TAsset>
    inline asset::Ptr<TAsset> AssetDB::Load(std::string_view virtual_path)
    {
        const std::string key(virtual_path);

        FilePtr file = ms_vfs->Open(key, FileMode::Read);
        if (!file)
        {
            PHX_CORE_ERROR("AssetDB: failed to open '{0}'", virtual_path);
            return asset::Ptr<TAsset>();
        }

        std::string content(file->GetSize(), '\0');
        file->Read(content.data(), content.size());

        auto result = rfl::yaml::read<TAsset>(content);
        if (!result)
        {
            PHX_CORE_ERROR(
                "AssetDB: failed to parse '{0}': {1}",
                virtual_path,
                result.error().what());

            return asset::Ptr<TAsset>();
        }

        TAsset* raw = new TAsset(std::move(*result));
        UUID asset_id;
        {
            std::scoped_lock _(ms_cache_mutex);

            auto it = ms_cache.find(key);
            if (it != ms_cache.end())
            {
                delete raw;
                return asset::Ptr<TAsset>(static_cast<TAsset*>(it->second.asset.get()));
            }

            ms_cache.emplace(key, AssetSlot{
                .asset = {  raw,
                            [](void *p) { delete static_cast<TAsset *>(p); }
                        },
                .asset_id = asset_id,
            });
        }

        return asset::Ptr<TAsset>(raw);
    }
}