#include "PhxAsset_pch.h"

#include "AssetDatabase.h"

using namespace phx;
using namespace phx::asset;

namespace 
{
    std::unique_ptr<IAssetLoader> g_asset_loader;
}

void AssetDB::Initialize(std::unique_ptr<IAssetLoader> loader)
{
    g_asset_loader = std::move(loader);
}

void* AssetDB::Find(std::string_view name, const reflect::TypeInfo &type_info)
{
    return nullptr;
}
