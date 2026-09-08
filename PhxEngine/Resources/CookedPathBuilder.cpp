#include "CookedPathBuilder.h"

#include <PhxEngine/Core/PathUtils.h>

namespace phx::resources
{
    std::string CookedPrefabPath(const std::string& source_path)
    {
        const std::string filename = GetFileNameWithoutExt(source_path);
        return "resources://prefabs/" + filename + ".phxfab";
    }

    std::string CookedMeshPath(const std::string& source_path, const std::string& sub_asset_name)
    {
        const std::string filename = GetFileNameWithoutExt(source_path);
        return "resources://meshes/" + filename + "_" + sub_asset_name + ".phxmsh";
    }

    std::string CookedTexturePath(const std::string& source_path, const std::string& sub_asset_name)
    {
        const std::string filename = GetFileNameWithoutExt(source_path);
        return "resources://textures/" + filename + "_" + sub_asset_name + ".phxtex";
    }

    std::string CookedMaterialPath(const std::string& source_path, const std::string& sub_asset_name)
    {
        const std::string filename = GetFileNameWithoutExt(source_path);
        return "resources://materials/" + filename + "_" + sub_asset_name + ".phxmtl";
    }
}
