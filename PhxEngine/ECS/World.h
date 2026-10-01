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
        T& Emplace(EntityId id, T&& component);

        template<typename T>
        [[nodiscard]] T* TryGet(EntityId id);

        template<typename T>
        [[nodiscard]] const T* TryGet(EntityId id) const;

        [[nodiscard]] bool IsEntityAlive(EntityId e) const;

        // -- Iterator ---
        template<SparseDriver TDriver, typename... TOthers, typename Fn>
        void Each(Fn&& fn);

        template<SparseDriver TDriver, typename... TOthers, typename Fn>
        void Each(Fn&& fn) const;

        template<SparseDriver TDriver>
        usize Count() const;

    private:
        template<typename T>
        auto& GetOrCreateStorage();

        template<typename T>
        auto& GetOrCreateStorage() const;

    private:
        const u32 m_num_component_types;

        // Lazily created on first use even from a const World -- Each/Count/
        // TryGet(id) const all need to reach a component's storage without
        // requiring it to already exist (e.g. Count<T>() on a component type
        // nothing has ever Emplace()'d yet should return 0, not assert).
        mutable std::unique_ptr<std::unique_ptr<IStorage>[]> m_component_storage;
        
        // -- Entity Pool (could be it's own class) ---
        std::vector<u32> m_entity_generation;
        std::vector<u32> m_free_indices;
    };

    template<typename T, typename... Args>
    inline T& World::Emplace(EntityId id, Args&&... args)
    {
        PHX_ASSERT(IsEntityAlive(id) && "Cannot Emplace a component onto a dead/stale entity");

        T& component = GetOrCreateStorage<T>().Emplace(id, std::forward<Args>(args)...);

        if constexpr (HasRequired<T>)
        {
            using TRequired = typename T::Required;
            if (!TryGet<TRequired>(id))
                Emplace<TRequired>(id);
        }

        return component;
    }

    template<typename T>
    inline T& World::Emplace(EntityId id, T&& component)
    {
        PHX_ASSERT(IsEntityAlive(id) && "Cannot Emplace a component onto a dead/stale entity");

        T& retVal = GetOrCreateStorage<T>().Emplace(id, std::forward<T>(component));

        if constexpr (HasRequired<T>)
        {
            using TRequired = typename T::Required;
            if (!TryGet<TRequired>(id))
                Emplace<TRequired>(id);
        }

        return retVal;
    }

    template<typename T>
    inline T* World::TryGet(EntityId id)
    {
        if (!IsEntityAlive(id))
            return nullptr;

        return GetOrCreateStorage<T>().TryGet(id);
    }

    template<typename T>
    inline const T* World::TryGet(EntityId id) const
    {
        if (!IsEntityAlive(id))
            return nullptr;

        return GetOrCreateStorage<T>().TryGet(id);
    }

    // TODO: Currently doesn't auto reorder based on sizes.
    // Should iterate the storage with least number of entries.
    // For now, will rely on user providing a good order.
    // Only alow sparse sets as the driver as linear would require every
    // entity that ever existed to be walked.
    template<SparseDriver TDriver, typename... TOthers, typename Fn>
    inline void World::Each(Fn&& fn)
    {
        SparseSet<TDriver>& driver = GetOrCreateStorage<TDriver>();
        Span<TDriver> driver_dense_map = driver.GetDenseMap();
        Span<EntityId> entities = driver.GetEntities();

        for (usize i = 0; i < entities.Size(); ++i)
        {
            const EntityId e = entities[i];
            if ((GetOrCreateStorage<TOthers>().Has(e) && ...))
                fn(e, driver_dense_map[i], *GetOrCreateStorage<TOthers>().TryGet(e)...);
        }
    }

    template<SparseDriver TDriver, typename... TOthers, typename Fn>
    inline void World::Each(Fn&& fn) const
    {
        const SparseSet<TDriver>& driver = GetOrCreateStorage<TDriver>();
        Span<TDriver> driver_dense_map = driver.GetDenseMap();
        Span<EntityId> entities = driver.GetEntities();

        for (usize i = 0; i < entities.Size(); ++i)
        {
            const EntityId e = entities[i];
            if ((GetOrCreateStorage<TOthers>().Has(e) && ...))
                fn(e, driver_dense_map[i], *GetOrCreateStorage<TOthers>().TryGet(e)...);
        }
    }

    template<SparseDriver TDriver>
    inline usize World::Count() const
    {
        const SparseSet<TDriver>& driver = GetOrCreateStorage<TDriver>();
        return driver.Size();
    }

    template<typename T>
    inline auto& World::GetOrCreateStorage()
    {
        using Storage = StorageTypeOf_t<T>;

        const u32 id = T::ID;
        PHX_ASSERT(id < m_num_component_types);

        if (!m_component_storage[id])
            m_component_storage[id] = std::make_unique<Storage>();

        return *static_cast<Storage*>(m_component_storage[id].get());
    }

    template<typename T>
    inline auto& World::GetOrCreateStorage() const
    {
        using Storage = StorageTypeOf_t<T>;

        const u32 id = T::ID;
        PHX_ASSERT(id < m_num_component_types);

        if (!m_component_storage[id])
            m_component_storage[id] = std::make_unique<Storage>();

        return *static_cast<const Storage*>(m_component_storage[id].get());
    }

}  // namespace phx::ecs