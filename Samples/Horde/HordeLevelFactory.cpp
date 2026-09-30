#include "HordeLevelFactory.h"

#include "WorldComponents.h"

#include <PhxEngine/Core/PhxDefines.h>

using namespace horde;
using namespace phx;

void horde::BuildBlockoutLevel(phx::ecs::World& world)
{
    // -- Camera ---
    const ecs::EntityId camera = world.CreateEntity();
    world.Emplace<TransformComponent>(camera, hlslpp::float3(0.0f, 15.0f, -20.0f));
    world.Emplace<CameraComponent>(camera, hlslpp::float3(0.0f, 0.0f, 0.0f));

    // -- Ground plane ---
    const ecs::EntityId ground = world.CreateEntity();
    world.Emplace<TransformComponent>(ground, hlslpp::float3(0.0f, 0.0f, 0.0f));
    world.Emplace<PlaneRenderComponent>(ground, hlslpp::float2(50.0f, 50.0f));

    // -- Capsule props ---
    const hlslpp::float3 capsule_positions[] = {
        hlslpp::float3(-5.0f, 1.0f,  3.0f),
        hlslpp::float3( 0.0f, 1.0f, -4.0f),
        hlslpp::float3( 6.0f, 1.0f,  2.0f),
    };

    for (const hlslpp::float3& position : capsule_positions)
    {
        const ecs::EntityId capsule = world.CreateEntity();
        world.Emplace<TransformComponent>(capsule, position);
        world.Emplace<CapsuleRenderComponent>(capsule, 0.5f, 2.0f); // radius, height
    }

    // -- Box obstacles ---
    const hlslpp::float3 box_positions[] = {
        hlslpp::float3(-3.0f, 0.5f, -2.0f),
        hlslpp::float3( 4.0f, 0.5f,  5.0f),
    };

    for (const hlslpp::float3& position : box_positions)
    {
        const ecs::EntityId box = world.CreateEntity();
        world.Emplace<TransformComponent>(box, position);
        world.Emplace<BoxRenderComponent>(box, hlslpp::float3(1.0f, 1.0f, 1.0f)); // extent
    }

    // -- Player ---
    const ecs::EntityId player = world.CreateEntity();
    world.Emplace<TransformComponent>(player, hlslpp::float3(0.0f, 1.0f, 0.0f));
    world.Emplace<CapsuleRenderComponent>(player, 0.5f, 1.8f);
    world.Emplace<PlayerTagComponent>(player);

    // -- Enemies ---
    const hlslpp::float3 enemy_positions[] = {
        hlslpp::float3(-6.0f, 1.0f,  6.0f),
        hlslpp::float3( 6.0f, 1.0f, -6.0f),
    };

    for (const hlslpp::float3& position : enemy_positions)
    {
        const ecs::EntityId enemy = world.CreateEntity();
        world.Emplace<TransformComponent>(enemy, position);
        world.Emplace<CapsuleRenderComponent>(enemy, 0.5f, 1.8f);
        world.Emplace<EnemyTagComponent>(enemy);
    }
}
