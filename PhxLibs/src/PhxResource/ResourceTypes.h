#pragma once

#include <PhxCore/Handle.h>
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

    struct ResourceHotData
    {
        ResourceState state = ResourceState::Unloaded;
    };

    struct ResourceColdData
    {
        std::atomic_uint32_t ref_count = 0;
        std::string source_path;
        std::string debug_name;
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

    struct GenericHandle
    {
        uint16_t type_id = 0;
        uint16_t generation = 0;
        uint16_t index = 0;

        bool IsValid() const { return generation != 0; }

        template<typename T>
        static GenericHandle From(Handle<T> h)
        {
            struct Raw { uint16_t idx; uint16_t gen; };
            Raw raw = std::bit_cast<Raw>(h);

            return {
                .type_id    = ResourceTypeId<T>::Get(),
                .generation = raw.gen,
                .index      = raw.idx };
        }

        template<typename T>
        Handle<T> To() const
        {
            PHX_CORE_ASSERT(type_id == ResourceTypeId<T>::Get(), "Invalid handle usage.");
            if (type_id != ResourceTypeId<T>::Get())
            {
                return Handle<T>();

            }
            struct Raw { uint16_t idx; uint16_t gen; };
            Raw raw = { index, generation };
            return std::bit_cast<Handle<T>>(raw);
        }
    };

}