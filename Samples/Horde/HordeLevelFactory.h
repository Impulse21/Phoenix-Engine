#pragma once

#include <PhxEngine/ECS/World.h>

namespace horde
{
    void BuildBlockoutLevel(phx::ecs::World& world);

    // -- Simple test for debugging GPU memory issues ---
    void BuildPrimitiveTests(phx::ecs::World& world);
}