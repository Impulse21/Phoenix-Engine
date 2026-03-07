#include "PhxAsset_pch.h"

#include <PhxCore/Reflect/TypeInfo.h>

#include <PhxAsset/YamlAssetWriter.h>

#include <yaml-cpp/yaml.h>

using namespace phx;
using namespace phx::asset;

YamlAssetWriter::YamlAssetWriter(IVirtualFileSystem *vfs)
    : m_vfs(vfs)
{
}

bool phx::asset::YamlAssetWriter::Write(std::string_view path, const reflect::TypeInfo &type_info, const void *asset)
{
    phx::Result<std::string> physical_path_result = m_vfs->ResolveVirtualToPhysicalPath(path);

    if (physical_path_result.HasError())
        return false;

    std::ofstream stream(physical_path_result->c_str());

    return true;
}

void phx::asset::YamlAssetWriter::WriteStruct(std::ostream &out, const reflect::TypeInfo &type_info, const void *asset)
{
}

void phx::asset::YamlAssetWriter::WriteArray(std::ostream &out, const reflect::TypeInfo &type_info, const void *vec_ptr, int indent)
{
}
