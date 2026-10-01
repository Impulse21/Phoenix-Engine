#include "HordeLevelFactory.h"

#include "WorldComponents.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/CVar.h>

PHX_CVAR_BOOL(include_enemies, true, "Include enemies in the level");

using namespace horde;
using namespace phx;

namespace
{
    inline hlslpp::float3 compute_camera_offset(float pitch_deg, float yaw_deg, float dist)
    {
        const float pitch = radians(hlslpp::float1(pitch_deg)).x;
        const float yaw   = radians(hlslpp::float1(yaw_deg)).x;

        const float horizontal = cos(hlslpp::float1(pitch)).x * dist;

        return hlslpp::float3(
            horizontal * sin(hlslpp::float1(yaw)).x,   // x
            sin(hlslpp::float1(pitch)).x * dist,       // y
            horizontal * cos(hlslpp::float1(yaw)).x);  // z
    }
}

void horde::BuildBlockoutLevel(phx::ecs::World& world)
{
    const hlslpp::float3 player_start_position = hlslpp::float3(0.0f, 1.5f, 0.0f);
    const hlslpp::float3 camera_offset         = compute_camera_offset(55.0f, 45.0f, 18.0f);

    // -- Camera ---
    const ecs::EntityId camera = world.CreateEntity();
    world.Emplace<TransformComponent>(camera, { .position = player_start_position + camera_offset});
    world.Emplace<CameraComponent>(
        camera,
        {
            .target = player_start_position,
            .fov_y_degrees = 40.0f,
            .near_plane = 5.0f,
            .far_plane = 80.0f,
        }
    );

    // -- Ground plane ---
    const ecs::EntityId ground = world.CreateEntity();
    world.Emplace<TransformComponent>(ground, { .position = hlslpp::float3(0.0f, 0.0f, 0.0f) });
    world.Emplace<PlaneRenderComponent>(ground, { .extent = hlslpp::float2(50.0f, 50.0f) });

    // -- Capsule props ---
    const hlslpp::float3 capsule_positions[] = {
        hlslpp::float3(-5.0f, 1.0f,  3.0f),
        hlslpp::float3( 0.0f, 1.0f, -4.0f),
        hlslpp::float3( 6.0f, 1.0f,  2.0f),
    };

    for (const hlslpp::float3& position : capsule_positions)
    {
        const ecs::EntityId capsule = world.CreateEntity();
        world.Emplace<TransformComponent>(capsule, { .position = position });
        world.Emplace<CapsuleRenderComponent>(capsule, { .radius = 0.5f, .height = 2.0f });
    }

    // -- Box obstacles ---
    const hlslpp::float3 box_positions[] = {
        hlslpp::float3(-3.0f, 0.5f, -2.0f),
        hlslpp::float3( 4.0f, 0.5f,  5.0f),
    };

    for (const hlslpp::float3& position : box_positions)
    {
        const ecs::EntityId box = world.CreateEntity();
        world.Emplace<TransformComponent>(box, { .position = position });
        world.Emplace<BoxRenderComponent>(box, { .extent = hlslpp::float3(1.0f, 1.0f, 1.0f) });
    }

    // -- Player ---
    const ecs::EntityId player = world.CreateEntity();
    world.Emplace<TransformComponent>(player, { .position =  player_start_position});
    world.Emplace<CapsuleRenderComponent>(player, { .radius = 0.5f, .height = 1.8f });
    world.Emplace<PlayerTagComponent>(player);

    // -- Enemies ---
    const hlslpp::float3 enemy_positions[] = {
        hlslpp::float3(-6.0f, 1.0f,  6.0f),
        hlslpp::float3( 6.0f, 1.0f, -6.0f),
    };

    for (const hlslpp::float3& position : enemy_positions)
    {
        const ecs::EntityId enemy = world.CreateEntity();
        world.Emplace<TransformComponent>(enemy, { .position = position });
        world.Emplace<CapsuleRenderComponent>(enemy, { .radius = 0.5f, .height = 1.8f });
        world.Emplace<EnemyTagComponent>(enemy);
    }
}

void horde::BuildPrimitiveTests(phx::ecs::World& world)
{
    // -- Camera ---
    const ecs::EntityId camera = world.CreateEntity();
    world.Emplace<TransformComponent>(camera, hlslpp::float3(0.0f, 15.0f, -20.0f));
    world.Emplace<CameraComponent>(camera, hlslpp::float3(0.0f, 0.0f, 0.0f));

    // -- Ground plane ---
    const ecs::EntityId ground = world.CreateEntity();
    world.Emplace<TransformComponent>(ground, hlslpp::float3(0.0f, 0.0f, 0.0f));
    world.Emplace<PlaneRenderComponent>(ground, hlslpp::float2(50.0f, 50.0f));

    // -- Player ---
    const ecs::EntityId player = world.CreateEntity();
    world.Emplace<TransformComponent>(player, { .position = hlslpp::float3(0.0f, 1.0f, 0.0f) });
    world.Emplace<CapsuleRenderComponent>(player, 0.5f, 1.8f);
    world.Emplace<PlayerTagComponent>(player);

    // -- Enemies ---
    const hlslpp::float3 enemy_positions[] = {
        hlslpp::float3(2.0f, 1.0f,  0.0f),
        // hlslpp::float3( 6.0f, 1.0f, -6.0f),
    };

    if (CVar_include_enemies.Get())
    {
        for (const hlslpp::float3& position : enemy_positions)
        {
            const ecs::EntityId enemy = world.CreateEntity();
            world.Emplace<TransformComponent>(enemy, { .position = position });
            world.Emplace<CapsuleRenderComponent>(enemy, 0.5f, 1.8f);
            world.Emplace<EnemyTagComponent>(enemy);
        }
    }

#if false
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

#endif
}

