#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxRenderer/MeshResource.h>
#include <PhxRenderer/MaterialResource.h>

#include <string>
#include <vector>
#include <optional>
#include <variant>

#include <hlsl++.h>

namespace phx
{
    struct ManifestMeshInstance
    {
        std::string mesh_path;
        std::vector<std::string> material_paths;
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
            hlslpp::interop::float3 scale = hlslpp::float3{ 1.0f, 1.0f, 1.0f };
            hlslpp::interop::float4 rotation = hlslpp::float4{ 0.0f, 0.0f, 0.0f, 1.0f };
            hlslpp::interop::float3 translation = hlslpp::float3{ 0.0f, 0.0f, 0.0f };

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
        RefCountPtr<phx::renderer::MeshResource> mesh;
        std::vector<RefCountPtr<phx::renderer::MaterialResource>> materials;
    };

    struct PrefabResource;
    struct NestedPrefabData 
    {
        RefCountPtr<PrefabResource> prefab_handle;
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

            hlslpp::float3 scale = { 1.0f, 1.0f, 1.0f };
            hlslpp::quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
            hlslpp::float3 translation = { 0.0f, 0.0f, 0.0f };

            std::variant<
                EmptyNodeData,
                MeshNodeData,
                CameraNodeData,
                LightNodeData,
                NestedPrefabData> data;
        };

        std::vector<Node> nodes;

        void Dispose() override {};
        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override { return true; };

        PHX_DECLARE_RESOURCE(PrefabResource);
	};
}


PHX_DEFINE_RESOURCE(
    PrefabResource,                 // T
    ".phxfab",                      // Extension
    "PrefabLoader"                  // Loader ID
);
