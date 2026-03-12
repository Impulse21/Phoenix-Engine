#pragma once

#include <PhxAsset/IAssetLoader.h>
#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Reflect/TypeInfo.h>

namespace YAML
{
    class Node;
}
namespace phx::asset
{
    class YamlAssetLoader : public IAssetLoader
    {
    public:
        explicit YamlAssetLoader(IVirtualFileSystem* vfs);

        bool Load(std::string_view path, const reflect::TypeInfo& type_info, void* out) const override;
        bool Exists(std::string_view path) const override;
        bool UseCache() const override { return true;}

    private:
        void ReadStruct(const YAML::Node& yaml_node, const reflect::TypeInfo& type_info, void* out_ptr) const;
        void ReadField(const YAML::Node& yaml_node, const reflect::FieldInfo& field_info, void* out_ptr) const;

        void ReadAssetRef(const YAML::Node& yaml_node, const reflect::FieldInfo& field_info ,void* out_ptr)
        void ReadFloatN(const YAML::Node& yaml_node, int n, void* out_ptr);

        void ReadArray(const YAML::Node& yaml_node, const reflect::FieldInfo& field_info, void* out_ptr)
    {
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

        bool UseCache() const override { return false;}
    }
}