#pragma once

#include <string>
namespace phx
{
    namespace CookedPathBuilder
    {
        std::string ForPrefab(const std::string& source_path);
        std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name); 
        std::string ForTexture(const std::string& source_path, const std::string& sub_asset_name);
        std::string ForMaterial(const std::string& source_path, const std::string& sub_asset_name);
    }
}