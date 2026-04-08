#pragma once

#include <PhxCore/Reflect/Reflection.h>

#include <string>
#include <vector>
#include <optional>

#include <hlsl++.h>

#include <PhxCore/Math.h> 

namespace phx::world::asset
{

    struct MeshInstanceDef
    {
        PHX_DECLARE_REFLECT(MeshInstanceDef)

        std::string mesh_path;
        std::vector<std::string> material_paths;
    };

    namespace LightTypeIds
    {
        constexpr const char* Point = "Point";
        constexpr const char* Spot = "Spot";
        constexpr const char* Directional = "Directional";
    }

    struct LightDef 
    {
        PHX_DECLARE_REFLECT(LightDef)

        std::string type; // "point", "spot", "directional"
        math::Float3 colour;
        float intensity;
    };

    namespace CameraTypeIds
    {
        constexpr const char* Perspective = "Perspective";
        constexpr const char* Orthographic = "Orthographic";
    }

    struct CameraDef
    {
        PHX_DECLARE_REFLECT(CameraDef)

        std::string type; // "perspective" or "orthographic"
        float fov_y;
        float z_near;
        float z_far;
    };

    namespace NodeTypeIds
    {
        constexpr const char* Empty = "Empty";
        constexpr const char* Mesh = "Mesh";
        constexpr const char* Camera = "Camera";
        constexpr const char* Light = "Light";
        constexpr const char* Prefab = "Prefab";
    }

    struct PrefabDef
    {
        PHX_DECLARE_REFLECT(PrefabDef)

        struct NodeDef
        {
            PHX_DECLARE_REFLECT(NodeDef)
            std::string name;
            int parent_index = -1;
            math::Float3 scale = math::Float3{ 1.0f, 1.0f, 1.0f };
            math::Float4 rotation = math::Float4{ 0.0f, 0.0f, 0.0f, 1.0f };
            math::Float3 translation = math::Float3{ 0.0f, 0.0f, 0.0f };

            std::string node_type;

            std::optional<MeshInstanceDef> mesh_instance_data;
            std::optional<CameraDef> camera_data;
            std::optional<LightDef> light_data;
            std::optional<std::string> nested_prefab_path;
        };

        std::vector<NodeDef> nodes;
    };
}