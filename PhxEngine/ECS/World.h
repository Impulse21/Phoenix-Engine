#pragma once

#include "SparseSetStorage.h"
#include "SingletonStorage.h"

#include <vector>
#include <memory>

namespace phx::ecs
{
    class World
    {
    public:
        explicit World(u32 num_components) noexcept
            : m_storage_sparse(num_components)
            , m_storage_singleton(num_components)
        {}

        EntityId CreateEntity();
        void FreeEntity(EntityId e);

    private:
        std::vector<std::unique_ptr<IStorage>> m_storage_sparse;
        std::vector<std::unique_ptr<IStorage>> m_storage_singleton;
        
        std::vector<u32> m_entity_generation;
        std::vector<u32> m_free_indices;
    };
}