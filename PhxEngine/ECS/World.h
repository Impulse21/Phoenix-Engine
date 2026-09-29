#pragma once

#include "SparseSet.h"
#include "SingletonStorage.h"
#include "LinearStorage.h"

#include "TypeTraits.h"

#include <vector>
#include <memory>

namespace phx::ecs
{
    class World
    {
    public:
        explicit World(u32 num_components_types) noexcept
            : m_num_component_types(num_components_types)
            , m_component_storage(
                std::make_unique<std::unique_ptr<IStorage>[]>(num_components_types))
        {
        }

        EntityId CreateEntity();
        void FreeEntity(EntityId e);

        template<typename T, typename... Args>
        T& Emplace(EntityId id, Args&&... args);

        template<typename T>
        [[nodiscard]] T* TryGet(EntityId id);

        [[nodiscard]] bool IsEntityAlive(EntityId e);

    private:
        template<typename T>
        auto& GetOrCreateStorage();

    private:
        const u32 m_num_component_types;

        std::unique_ptr<std::unique_ptr<IStorage>[]> m_component_storage;
        
        // -- Entity Pool (could be it's own class) ---
        std::vector<u32> m_entity_generation;
        std::vector<u32> m_free_indices;
    };

    template<typename T, typename... Args>
    inline T& World::Emplace(EntityId id, Args&&... args)
    {
        auto& storage = GetOrCreateStorage<T>().Emplace(id, std::forward<Args>(args)...);

        if constexpr (HasRequired<T>))
        {
            using TRequired = typename T::Required;
            Emplace<TRequired>(id);
        }
    }

    template<typename T>
    inline T* World::TryGet(EntityId id)
    {
        if (!IsEntityAlive(id))
            return nullptr;

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
    inline auto& World::GetOrCreateStorage()
    {
        using Storage = StorageTypeOf_t<T>;
        
        const u32 id = T::ID
        PHX_ASSERT(id < m_num_component_types);


        if (!component_storage[id])
            component_storage[id] = std::make_unique<Storage>();

        return *static_cast<Storage*>(component_storage[id].get());
    }

}  // namespace phx::ecs