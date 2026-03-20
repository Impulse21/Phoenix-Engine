#pragma once

#include <string>
#include <PhxCore/UUID.h>

#define PHX_DEFINE_ASSET(type)                                              \
    phx::asset::AssetHeader header = { .asset_type = #type, /*.asset_id = {}*/}

namespace phx::asset
{
    struct AssetHeader
    {
        std::string asset_type;
        // Could not get serlaization to work with this. So annyoing.
        /* UUID asset_id; */
    };
}