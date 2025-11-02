#pragma once

#include <string>
#include <vector>
#include <optional>

#include <hlsl++.h>

#include <PhxResource/Resource.h>

namespace phx
{
    struct ManifestMeshInstance
    {
        std::string mesh_path;
        std::optional<std::string> material_path;
    };

    struct ManifestLightData 
    {
        std::string type; // "point", "spot", "directional"
        hlslpp::interop::float3 colour;
        float intensity;
    };

    struct ManifestCameraData 
    {
        std::string type; // "perspective" or "orthographic"
        float fovY;
        float z_near;
        float z_far;
        // ... other camera props
    };

    namespace NodeTypeIds
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

	struct PrefabResource final : public Resource
	{
        struct Node 
        {
            std::string name;
            int parent_index;

            // Handles to the *actual* loaded resources
            RefCountPtr<Resource> mesh;
            RefCountPtr<Resource> material;
            RefCountPtr<Resource> nested_prefab;
        };

        std::vector<Node> nodes;

        ~PrefabResource() override = default;

        PHX_DECLARE_RESOURCE(PrefabResource)
	};

    struct PrefabHandleResource final : public Resource
    {
        RefCountPtr<PrefabResource> prefab;

        ~PrefabHandleResource() override = default;
        PHX_DECLARE_RESOURCE(PrefabHandleResource)
	};
}