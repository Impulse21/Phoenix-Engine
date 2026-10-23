#pragma once

#include <PhxEngine/ECS/World.h>

namespace horde
{
    // Builds block out level and returns main camera
    phx::ecs::EntityId BuildBlockoutLevel(phx::ecs::World& world);

    // Builds prmitive test level and returns main camera
    phx::ecs::EntityId BuildPrimitiveTests(phx::ecs::World& world);
}