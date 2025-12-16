#pragma once

#include <PhxCore/Pool.h>
#include <PhxCore/Assert.h>

#include "ResourceTypeTraits.h"
#include "ResourceTypes.h"

namespace phx
{
    template<typename T>
    class ResourceStore
    {
    public:
        // Use Traits to find types
        using HotType = typename ResourceTraits<T>::Hot;
        using ColdType = typename ResourceTraits<T>::Cold;

        static void Initialize(uint16_t capacity)
        {
            PHX_ASSERT(!s_initialized);
            s_pool.Initialize(capacity);
            s_initialized = true;
        }

        static void Shutdown() 
        { 
            s_pool.Shutdown(); 
            s_initialized = false; 
        }

        // --- Ref Counting Callbacks ---
        static void IncRefGeneric(GenericHandle h)
        {
            auto* cold = s_pool.GetCold(h.index, h.generation);
            if (cold) 
                cold->ref_count.fetch_add(1, std::memory_order_relaxed);
        }

        static void DecRefGeneric(GenericHandle h)
        {
            auto* cold = s_pool.GetCold(h.index, h.generation);

            if (cold) 
                cold->ref_count.fetch_sub(1, std::memory_order_relaxed);
        }

        static bool IsLoadedGeneric(GenericHandle h)
        {
            auto* hot = s_pool.GetHot(h.index, h.generation);
            if (hot)
                return hot->state == ResourceState::Loaded;

			return false;
        }
        
        static bool IsErrorStateGeneric(GenericHandle h)
        {
            auto* hot = s_pool.GetHot(h.index, h.generation);
            if (hot)
                return hot->state == ResourceState::Error;

            return false;
        }

        static void SetStateGeneric(GenericHandle h, ResourceState resource_state)
        {
            auto* hot = s_pool.GetHot(h.index, h.generation);
            if (hot)
            {
                hot->state = resource_state;
            }
        }

        // --- Accessors ---
        static Handle<T> Allocate()
        {
            PHX_ASSERT(s_initialized);
            return s_pool.Allocate();
        }

        static HotType* GetHot(Handle<T> h) { return s_pool.GetHot(h); }
        static ColdType* GetCold(Handle<T> h) { return s_pool.GetCold(h.index, h.generation); } // Use overload

        static PagedPool<Handle<T>, HotType, ColdType>& GetPool() { return s_pool; }

    private:
        inline static PagedPool<T, HotType, ColdType> s_pool;
        inline static bool s_initialized = false;
    };
}