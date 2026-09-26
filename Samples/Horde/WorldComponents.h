#pragma once

#include <PhxEngine/ECS/ComponentPolicy.h>

#include <hlsl++.h>

namespace horde
{
    namespace FixComponentId
    {
        enum : u32
        {
            Transform = 0,
            CapsuleRenderComponent,
            PlaneRenderComponent,
            EnvProperties,
        };
    }

    struct TransformComponent
    {
        static constexpr int ID = FixComponentId::Transform;

        hlslpp::float3      position;
        hlslpp::quaternion  rotation;
        hlslpp::float3      scale;
    };

    struct CapsuleRenderComponent
    {
        static constexpr int ID = FixComponentId::CapsuleRenderComponent;
        using Required = TransformComponent;

        float radius = 0.0f;
        float height = 0.0f;
    };

    struct PlaneRenderComponent
    {
        static constexpr int ID = FixComponentId::PlaneRenderComponent;
        using Required = TransformComponent;
        
        hlslpp::interop::float2 extent = {};
    };

    struct EnvPropertiesComponent
    {
        static constexpr int ID = FixComponentId::EnvProperties;
        
        bool simple_test_bool = false;
    };
}

PHX_SINGLETON_COMPONENT(horde::EnvPropertiesComponent);