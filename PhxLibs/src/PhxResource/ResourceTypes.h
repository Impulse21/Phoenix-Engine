#pragma once

#include <PhxCore/Handle.h>
#include <atomic>

namespace phx
{
    namespace internal
    {
        struct ResourceTypeIdCounter
        {
            static inline std::atomic_uint16_t value = 0;
        };
    }

    template<typename T>
    struct ResourceTypeId
    {
        static uint16_t Get()
        {
            static uint16_t id = internal::ResourceTypeIdCounter::value.fetch_add(1);
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
            return GenericHandle{
                .type_id = ResourceTypeId<T>::Get(),
                .generation = h.GetGeneration(),
                .index = h.GetIndex()
            };
        }

        // 2. Convert TO a typed Handle
        // This validates the Type ID for safety.
        template<typename T>
        Handle<T> To() const
        {
            if (type_id != ResourceTypeId<T>::Get())
            {
                return Handle<T>();

            }
            return Handle<T>::FromIndex(index, generation);
        }
    };
}