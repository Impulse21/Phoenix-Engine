#pragma once

#include <string>
#include <vector>

#include <PhxResource/Resource.h>

namespace phx
{
    // -- on disk representation ---
    struct PrefabManifest
    {
        struct Node
        {
            std::string name;
            int parent_index;

            std::string mesh_path;
            std::string material_path;
            std::string nested_prefab_path;
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