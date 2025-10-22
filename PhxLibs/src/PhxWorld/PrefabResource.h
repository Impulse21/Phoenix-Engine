#pragma once

#include <string>
#include <vector>

#include <PhxResource/Resource.h>

namespace phx
{
    // Goes into the GLTF handler
    struct PrefabFile
    {
        struct Node
        {
            std::string name;
            int parent_index;

            std::string mesh_path;
            std::string material_path;
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
        };

        std::vector<Node> nodes;

        ~PrefabResource() override = default;

        PHX_DECLARE_RESOURCE(PrefabResource)

	};
}