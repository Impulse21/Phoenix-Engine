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

        // -- Iterator ---
        template<SparseDriver TDriver, typename... TOthers, typename Fn>
        void Each(Fn&& fn);

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
    inline T* World::TryGet(EntityId id)
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
        for (auto& e : driver.GetEntities())
        {
            if ((GetOrCreateStorage<TOthers>().Has(e) && ...))
                fn(e, driver_dense_map[e.Index()], *GetOrCreateStorage<TOthers>().TryGet(e)...);
        }
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

}  // namespace phx::ecs