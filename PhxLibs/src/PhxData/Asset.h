#pragma once

#include <cstdint>

#define PHX_DECLARE_ASSET(TYPE, BASE)                                       \
public:                                                                     \
    static constexpr uint64_t StaticTypeHash() { return StringHash(#TYPE); } \
    TYPE() : BASE(StaticTypeHash()) {}

namespace phx::data
{
    // The base for all high-level, user-facing assets.
    struct Asset
    {
        const uint64_t asset_type_hash;

        // The virtual destructor is essential for safe polymorphic storage
        // in the AssetManager's cache.
        virtual ~Asset() = default;

    protected:
        explicit Asset(uint64_t hash) : asset_type_hash(hash) {}
    };
}