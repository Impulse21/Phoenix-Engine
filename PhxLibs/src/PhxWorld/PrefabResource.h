#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>

#include <hlsl++.h>

#include <PhxResource/Resource.h>

namespace phx
{
    struct ManifestMeshInstance
    {
        std::string mesh_path;
        std::optional<std::string> material_path;
    };

    namespace ManifiestLightTypeIds
    {
        constexpr const char* Point = "Point";
        constexpr const char* Spot = "Spot";
        constexpr const char* Directional = "Directional";
    }

    struct ManifestLightData 
    {
        std::string type; // "point", "spot", "directional"
        hlslpp::interop::float3 colour;
        float intensity;
    };

    namespace ManifiestCameraTypeIds
    {
        constexpr const char* Perspective = "Perspective";
        constexpr const char* Orthographic = "Orthographic";
    }

    struct ManifestCameraData 
    {
        std::string type; // "perspective" or "orthographic"
        float fov_y;
        float z_near;
        float z_far;
    };

    namespace ManifiestNodeTypeIds
    {
        constexpr const char* Empty = "Empty";
        constexpr const char* Mesh = "Mesh";
        constexpr const char* Camera = "Camera";
        constexpr const char* Light = "Light";
        constexpr const char* Prefab = "Prefab";
    }

    // -- on disk representation ---
    struct PrefabManifest
    {
        struct Node
        {
            std::string name;
            int parent_index = -1;
            hlslpp::interop::float4x4 local_transform;

            std::string node_type; // e.g., "Empty", "Mesh", "Camera", "Light", "Prefab"

            // Optional payloads. Only one of these will be set.
            // std::optional is perfect here because it's clean and
            // nlohmann::json knows how to serialize it (it's null if empty).
            std::optional<ManifestMeshInstance> mesh_instance_data;
            std::optional<ManifestCameraData> camera_data;
            std::optional<ManifestLightData> light_data;
            std::optional<std::string> nested_prefab_path;
        };

        std::vector<Node> nodes;
    };

    struct EmptyNodeData {};

    struct MeshNodeData 
    {
        RefCountPtr<Resource> mesh;
        RefCountPtr<Resource> material;
    };

    struct NestedPrefabData 
    {
        RefCountPtr<Resource> prefabHandle;
    };

    struct CameraNodeData 
    {
        enum class Type { Perspective, Orthographic };
        Type type;
        float fov_y;
        float z_near;
        float z_far;
    };

    struct LightNodeData 
    {
        enum class Type { Point, Spot, Directional };
        Type type;
        hlslpp::float3 colour;
        float intensity;
    };

	struct PrefabResource final : public Resource
	{
        struct Node 
        {
            std::string name;
            int parent_index;
            hlslpp::float4x4 local_transform;

            std::variant<
                EmptyNodeData,
                MeshNodeData,
                CameraNodeData,
                LightNodeData,
                NestedPrefabData> data;
        };

        std::vector<Node> nodes;

        ~PrefabResource() override = default;

        bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork>, size_t&) override { return false; }
        PHX_DECLARE_RESOURCE(PrefabResource)
	};

    struct PrefabHandleResource final : public Resource
    {
        RefCountPtr<PrefabResource> prefab;

        PHX_DECLARE_RESOURCE(PrefabHandleResource);

        ~PrefabHandleResource() override = default;

        bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> /*transitions*/, size_t& /*fill_index*/) override
        {
            return false;
        };
	};
}