#pragma once

#include <cstdint>

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#define PHX_DECLARE_ASSET(TYPE)                                       \
public:                                                                     \
    static constexpr uint64_t StaticTypeHash() { return StringHash(#TYPE); } \
    TYPE() : Asset(StaticTypeHash()) {}

namespace phx::data
{
    // The base for all high-level, user-facing assets.
    struct Asset : public RefCounted
    {
        enum State : uint8_t
        {
            Loaded = 0,
            Loading = 0x0F,
            Error = 0x7F,
            Unloaded = 0xFF
        };

        std::atomic_uint8_t State = State::Unloaded;
        const uint64_t asset_type_hash;

        // The virtual destructor is essential for safe polymorphic storage
        // in the AssetManager's cache.
        virtual ~Asset() = default;

    protected:
        explicit Asset(uint64_t hash) : asset_type_hash(hash) {}
    };
}