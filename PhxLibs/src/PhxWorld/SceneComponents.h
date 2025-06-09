#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>

#include <hlsl++.h>

#define PHX_DECLARE_COMPONENT(TYPE, BASE)                                           \
    public:                                                                         \
                                                                                    \
        static constexpr uint64_t StaticTypeHash() { return StringHash(#TYPE); }    \
        TYPE() : BASE(StaticTypeHash()) {}


namespace phx
{
    struct Resource;
}

namespace phx::scene
{
    struct Component
    {
        const StringHash type_hash;

        virtual ~Component() = default;

    protected:
        explicit Component(uint64_t hash) : type_hash(hash) {}
    };

    struct MeshComponent : public Component
    {
        RefCountPtr<Resource> mesh;

        PHX_DECLARE_COMPONENT(MeshComponent, Component)
    };

    enum class LightType { Directional, Point, Spot };
    struct LightComponent : public Component
    {
        LightType light_type = LightType::Point;
        hlslpp::float3 colour = { 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float range = 0.0f;
        float inner_cone_angle = 0.0f;
        float outer_cone_angle = 0.0f;

        PHX_DECLARE_COMPONENT(LightComponent, Component)
    };

    struct MaterialComponent : public Component
    {
        RefCountPtr<Resource> material;

        PHX_DECLARE_COMPONENT(MaterialComponent, Component)
    };

    struct CameraComponent : public Component
    {
        float field_of_view = 70.0f;

        PHX_DECLARE_COMPONENT(CameraComponent, Component)
    };
}