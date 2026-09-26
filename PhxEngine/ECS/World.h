#pragma once

#include "SparseSetStorage.h"
#include "SingletonStorage.h"

#include "ComponentPolicy.h"

#include <vector>
#include <memory>

namespace phx::ecs
{
    class World
    {
    public:
        explicit World(u32 num_components_types) noexcept
            : m_num_component_types(num_components_types)
            , m_storage_sparse(
                std::make_unique<std::unique_ptr<IStorage>[]>(num_components_types))
            , m_storage_singleton(
                std::make_unique<std::unique_ptr<IStorage>[]>(num_components_types))
        {
        }

        EntityId CreateEntity();
        void FreeEntity(EntityId e);

        template<typename T, typename... Args>
        T& Emplace(EntityId id, Args&&... args);

        template<typename T>
        T* TryGet(EntityId id);

    private:
        template<typename T>
        SparseSetStorage<T>& GetOrCreateSparseStorage();

        template<typename T>
        SingletonStorage<T>& GetOrCreateSingletonStorage();

    private:
        const u32 m_num_component_types;

        std::unique_ptr<std::unique_ptr<IStorage>[]> m_storage_sparse;
        std::unique_ptr<std::unique_ptr<IStorage>[]> m_storage_singleton;
        
        // -- Entity Pool (could be it's own class) ---
        std::vector<u32> m_entity_generation;
        std::vector<u32> m_free_indices;
    };

    template<typename T, typename... Args>
    inline T& World::Emplace(EntityId id, Args&&... args)
    {
        if constexpr (is_singleton_v<T>)
        {
            return GetOrCreateSingletonStorage<T>().Emplace(std::forward<Args>(args)...);
        }
        else
        {
            return GetOrCreateSparseStorage<T>().Emplace(id, std::forward<Args>(args)...);
        }
    }

    template<typename T>
    inline T* World::TryGet(EntityId id)
    {
        if constexpr (is_singleton_v<T>)
        {
            SingletonStorage<T>& storage = GetOrCreateSingletonStorage<T>();
            return storage.Has() ? &storage.Get() : nullptr;
        }
        else
        {
            return GetOrCreateSparseStorage<T>().TryGet(id);
        }
    }

    template<typename T>
    inline SparseSetStorage<T>& World::GetOrCreateSparseStorage()
    {
        PHX_ASSERT(T::ID < m_num_component_types);

        if (m_storage_sparse[T::ID] == nullptr)
        {
            m_storage_sparse[T::ID] = std::make_unique<SparseSetStorage<T>>();
        }

        return *static_cast<SparseSetStorage<T>*>(m_storage_sparse[T::ID].get());
    }

    template<typename T>
    inline SingletonStorage<T>& World::GetOrCreateSingletonStorage()
    {
        PHX_ASSERT(T::ID < m_num_component_types);

        if (m_storage_singleton[T::ID] == nullptr)
        {
            m_storage_singleton[T::ID] = std::make_unique<SingletonStorage<T>>();
        }
        
        return *static_cast<SingletonStorage<T>*>(m_storage_singleton[T::ID].get());
    }

}  // namespace phx::ecs