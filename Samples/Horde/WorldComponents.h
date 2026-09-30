#pragma once

#include <PhxEngine/ECS/LinearStorage.h>
#include <PhxEngine/ECS/SingletonStorage.h>

#include <hlsl++.h>

namespace horde
{
    namespace WorldComponentId
    {
        enum : u32
        {
            Transform = 0,
            CapsuleRenderComponent,
            PlaneRenderComponent,
            BoxRenderComponent,
            PlayerTag,
            EnemyTag,
            EnvProperties,
            NumComponents,
        };
    }

    struct TransformComponent
    {
        using StorageType = phx::ecs::LinearStorage<TransformComponent>;
        static constexpr u32 ID = WorldComponentId::Transform;

        hlslpp::float3      position;
        hlslpp::quaternion  rotation = hlslpp::quaternion::identity();
        hlslpp::float3      scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
    };

    struct CapsuleRenderComponent
    {
        static constexpr u32 ID = WorldComponentId::CapsuleRenderComponent;
        using Required = TransformComponent;

        float radius = 0.0f;
        float height = 0.0f;
    };

    struct PlaneRenderComponent
    {
        static constexpr u32 ID = WorldComponentId::PlaneRenderComponent;
        using Required = TransformComponent;

        hlslpp::interop::float2 extent = {};
    };

    struct BoxRenderComponent
    {
        static constexpr u32 ID = WorldComponentId::BoxRenderComponent;
        using Required = TransformComponent;

        hlslpp::float3 extent = {};
    };

    // -- Tags: empty marker components, no render/physics data of their own ---
    struct PlayerTagComponent
    {
        static constexpr u32 ID = WorldComponentId::PlayerTag;
    };

    struct EnemyTagComponent
    {
        static constexpr u32 ID = WorldComponentId::EnemyTag;
    };

    struct EnvPropertiesComponent
    {
        using StorageType = phx::ecs::SingletonStorage<EnvPropertiesComponent>;
        static constexpr u32 ID = WorldComponentId::EnvProperties;

        bool simple_test_bool = false;
    };

    inline hlslpp::float4x4 ToMatrix(const TransformComponent& transform)
    {
        const hlslpp::float4x4 scale_m       = hlslpp::float4x4::scale(transform.scale);
        const hlslpp::float4x4 rotation_m    = hlslpp::float4x4(transform.rotation);
        const hlslpp::float4x4 translation_m = hlslpp::float4x4::translation(transform.position);
        return hlslpp::mul(scale_m, hlslpp::mul(rotation_m, translation_m));
    }
}