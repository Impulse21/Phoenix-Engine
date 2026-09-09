#pragma once

#include <string>

namespace phx::resources
{
    std::string CookedPrefabPath(const std::string& source_path);
    std::string CookedMeshPath(const std::string& source_path, const std::string& sub_asset_name);
    std::string CookedTexturePath(const std::string& source_path, const std::string& sub_asset_name);
    std::string CookedMaterialPath(const std::string& source_path, const std::string& sub_asset_name);
}
