#include "PhxAsset_pch.h"

#include <PhxCore/Reflect/TypeInfo.h>

#include <PhxAsset/AssetLoaders.h>

#include <yaml-cpp/yaml.h>

using namespace phx;
using namespace phx::asset;

bool YamlAssetLoader::Load(std::string_view path, const reflect::TypeInfo &type_info, void *out) const
{
    phx::Result<std::string> physical_path_result = m_vfs->ResolveVirtualToPhysicalPath(path);
    if (physical_path_result.HasError())
        return false;

    try
    {
        const YAML::Node root = YAML::LoadFile(physical_path_result.GetValue());
        ReadStruct(root, out, ti);
    }
    catch (const YAML::Exception &e)
    {
        PHX_CORE_ERROR("YamlLoader: failed to parse {0}: {1}", path, e.what());
        return false;
    }

    return true;
}

bool YamlAssetLoader::Exists(std::string_view path) const override
{
    return m_vfs->Exists(path);
}