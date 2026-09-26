#include "World.h"

using namespace phx::ecs;

EntityId World::CreateEntity()
{
    u32 index;

    if (!m_free_indices.empty())
    {
        // TODO:
    }
    else
    {
        // TODO: Add entry
    }



    const u32 generator_value = m_entity_generation[index];
    return MakeEntityId(index, generator_value);
}
