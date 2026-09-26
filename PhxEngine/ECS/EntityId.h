#pragma once

#include <PhxEngine/Core/PhxDefines.h>
namespace phx::ecs
{
    struct EntityId
    {
        static constexpr u32 k_index_mask           = 0x00FFFFFF;
        static constexpr u32 k_generator_mask       = 0x000000FF;
        static constexpr u32 k_generator_offset     = 24;

        static constexpr u32 Null = 0xFFFFFFFF;

        u32 value = Null;   // e.g. 24 bits index (16M entities) | 8 bits generation (256 recycles/slot)

        u32 Index()      const { return value & k_index_mask; }
        u32 Generation() const { return value >> k_generator_offset; }
    };

    inline EntityId MakeEntityId(u32 index, u32 generation)
    {
        return EntityId{
            .value = 
                ((generation & EntityId::k_generator_mask) << EntityId::k_generator_offset)
                | (index & EntityId::k_index_mask)
        };
    }
}