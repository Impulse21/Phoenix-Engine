#include "World.h"

using namespace phx::ecs;

EntityId World::CreateEntity()
{
    u32 index;

    if (!m_free_indices.empty())
    {
        index = m_free_indices.back();
        m_free_indices.pop_back();
    }
    else
    {
        index = m_entity_generation.size();
        m_entity_generation.emplace_back(0);
    }
    
    const u32 generator_value = m_entity_generation[index];

    return MakeEntityId(index, generator_value);
}

void phx::ecs::World::FreeEntity(EntityId e)
{
    PHX_ASSERT(e.Index() < m_entity_generation.size());

    const u32 index = e.Index();
    PHX_ASSERT(m_entity_generation[index] == e.Generation());

    for (u32 i = 0; i < m_num_component_types; ++i)
    {
        if (m_storage_sparse[i] != nullptr && m_storage_sparse[i]->Has(e))
        {
            m_storage_sparse[i]->Remove(e);
        }
    }
    
    m_entity_generation[index]++;
    m_free_indices.push_back(index);
}

bool phx::ecs::World::IsEntityAlive(EntityId e)
{
    PHX_ASSERT(e.Index() < m_entity_generation.size());
    const u32 index = e.Index();
    const u32 generation = e.Generation();

    return m_entity_generation[index] == generation;
}
