#include "PhxAsset_pch.h"

#include <PhxAsset/AssetDatabase.h>

#include <memory>
#include <mutex>
#include <unordered_map>

using namespace phx;
using namespace phx::asset;

namespace 
{
    struct AssetSlotDeleter
    {
        const reflect::TypeInfo* type_info;

        void operator()(void* p) const
        {
            type_info->destruct_place(p);
            free(p);
        }
    };

    struct AssetSlot
    {
        std::unique_ptr<void, AssetSlotDeleter> asset_ptr;
        const reflect::TypeInfo* type_info;
        std::vector<std::string> dependents;
    };

    std::unordered_map<std::string, AssetSlot> g_cache;
    std::mutex g_cache_mutex;
    std::unique_ptr<IAssetLoader> g_asset_loader;
}

void AssetDB::Initialize(std::unique_ptr<IAssetLoader> loader)
{
    g_asset_loader = std::move(loader);
}

void* AssetDB::Find(std::string_view path, const reflect::TypeInfo &type_info)
{
    std::string key(path);

    {
        std::scoped_lock _(g_cache_mutex);

        auto it = g_cache.find(key);
        if (it != g_cache.end())
            return it->second.asset_ptr.get();
    }

    void* raw = malloc(type_info.size);
    type_info.construct_place(raw);

    if (!g_asset_loader->Load(path, type_info, raw))
    {
        type_info.destruct_place(raw);
        free(raw);

        PHX_CORE_ERROR("Failed to load asset {0}", key);
        return nullptr;
    }
    {
        std::scoped_lock _(g_cache_mutex);

        auto it = g_cache.find(key);
        if (it != g_cache.end())
            return it->second.asset_ptr.get();

        g_cache[key] = AssetSlot
        {
            .asset_ptr = std::unique_ptr<void, AssetSlotDeleter>(raw, { &type_info }),
            .type_info = &type_info,
        };
    }

    return g_cache[key].asset_ptr.get();
}
