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
            EnvProperties,
            NumComponents,
        };
    }

    struct TransformComponent
    {
        using StorageType = phx::ecs::LinearStorage<TransformComponent>;
        static constexpr u32 ID = WorldComponentId::Transform;

        hlslpp::float3      position;
        hlslpp::quaternion  rotation;
        hlslpp::float3      scale;
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

    struct EnvPropertiesComponent
    {
        using StorageType = phx::ecs::SingletonStorage<EnvPropertiesComponent>;
        static constexpr u32 ID = WorldComponentId::EnvProperties;
        
        bool simple_test_bool = false;
    };
}