#include "PhxAsset_pch.h"

#include <PhxAsset/AssetDatabase.h>

#include <memory>
#include <mutex>
#include <unordered_map>

using namespace phx;
using namespace phx::asset;

namespace 
{
}

#if !USE_VENDOR_REFLECTION
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
#endif
