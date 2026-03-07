#pragma once

#include <PhxAsset/IAssetLoader.h>
#include <PhxCore/IVirtualFileSystem.h>

namespace phx::asset
{

    class YamlAssetLoader : public IAssetLoader
    {
    public:
        explicit YamlAssetLoader(IVirtualFileSystem* vfs);

        bool Load(std::string_view path, const reflect::TypeInfo& type_info, void* out) const override;
        bool Exists(std::string_view path) const override;

    private:
        void ReadStruct()
    private:
        IVirtualFileSystem* m_vfs;
    };


    class BakedAssetLoader : public IAssetLoader
    {
    public:
        BakedAssetLoader() = default;
        bool Load(std::string_view path, const reflect::TypeInfo& type_info, void* out) const override 
        { 
            return false;
        };

        bool Exists(std::string_view path) const override
        {
            return false;
        }
    }
}