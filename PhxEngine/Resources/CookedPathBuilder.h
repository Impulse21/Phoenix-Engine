#pragma once

#include <string>

namespace phx::resources
{
    // Cooked-asset paths, all under the "resources://" mount (see
    // ModelViewerApp.cpp's VFS::Mount("resources://", <assets>/.compiled)).
    // Deliberately flat -- no source subdirectory nesting -- so callers
    // never need to know the assets root, just VFS::ReadFile/WriteFile the
    // returned virtual path directly.
    std::string CookedPrefabPath(const std::string& source_path);
    std::string CookedMeshPath(const std::string& source_path, const std::string& sub_asset_name);
    std::string CookedTexturePath(const std::string& source_path, const std::string& sub_asset_name);
    std::string CookedMaterialPath(const std::string& source_path, const std::string& sub_asset_name);
}
