#pragma once

#include <PhxRhi/PhxRhi_Types.h>


#include <atomic>
#include <bit>
#include <string>

namespace phx
{
    enum ResourceState : uint8_t
    {
        Unloaded = 0,

        // AsyncLoader has started. 
        // Loaders can use [0x02 - 0x1F] for internal steps (e.g. Loading + 1)
        Loading = 0x01,

        Waiting_dependencies = 0x20,
        Copied_to_gpu = 0x40,
        Pending_gfx_transition = 0x50,
        Loaded = 0x60,

        Error = 0xFF
    };

    namespace internal
    {
        inline uint16_t GenerateNewTypeId()
        {
            // Reserve id 0 for invalid id's.
            static constinit std::atomic<uint16_t> s_IdCounter = 1;
            return s_IdCounter.fetch_add(1, std::memory_order_relaxed);
        }
    }

    template<typename T>
    struct ResourceTypeId
    {
        static uint16_t Get()
        {
            static uint16_t id = internal::GenerateNewTypeId();
            return id;
        }
    };

}