#pragma once

#include <PhxAsset/AssetPtr.h>

#include <PhxCore/Reflect/Reflection.h>

#include <rfl.hpp>
#include <rfl/yaml.hpp>

#include <memory>

namespace phx::asset
{
    // -- This is a static class as I require private fields for cache lookups.
    class AssetDB final
    {
    public:
        static void Initialize(IVirtualFileSystem* vfs)
        {
            ms_vfs = vfs;
        }

        template <typename TAsset>
        static AssetPtr<TAsset> Get(std::string_view virtual_file_path);

    private:
        AssetDB() = default;

    private:
        struct AssetSlot
        {
            std::unique_ptr<void> asset_ptr;
        };

        inline static phx::IVirtualFileSystem* ms_vfs = nullptr;
        inline static std::unordered_map<std::string, AssetSlot> ms_cache;
        inline static std::mutex ms_cache_mutex;
        inline static std::unique_ptr<IAssetLoader> ms_asset_loader;
    }

    template <typename TAsset>
    inline AssetPtr<TAsset> AssetDB::Get(std::string_view virtual_file_path)
    {
        std::string key(path);

        {
            std::scoped_lock _(ms_cache_mutex);

            auto it = ms_cache.find(key);
            if (it != ms_cache.end())
                return it->second.asset_ptr.get();
        }

        auto *vfs = IVirtualFileSystem::Ptr;

        FilePtr file = m_vfs->Open(std::string(path), FileMode::Read);
        if (!file)
            return {};

        const size_t size = file->GetSize();
        std::string content;
        content.resize(size);

        auto result = rfl::yaml::read<TAsset>(content);
        if (!result)
        {
            PHX_CORE_ERROR("Failed to parse yaml file: {0}", result.value());
        }

        
        {
            std::scoped_lock _(ms_cache_mutex);

            auto it = ms_cache.find(key);
            if (it != ms_cache.end())
                return it->second.asset_ptr.get();

            ms_cache[key] = AssetSlot{
                .asset_ptr = std::unique_ptr<void, AssetSlotDeleter>(raw, {&type_info}),
                .type_info = &type_info,
            };
        }

        return {static_cast<TAsset *>(ptr)};
    }
}