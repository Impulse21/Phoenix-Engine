#pragma once

#include <PhxEngine/Core/PhxDefines.h>
namespace phx::ecs
{
    struct EntityId
    {
        static constexpr u32 Null = 0xFFFFFFFF;

        u32 value = Null;   // e.g. 24 bits index (16M entities) | 8 bits generation (256 recycles/slot)

        u32 Index()      const { return value & 0x00FFFFFF; }
        u32 Generation() const { return value >> 24; }
    };
}