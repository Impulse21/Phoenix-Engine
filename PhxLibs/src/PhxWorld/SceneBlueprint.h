#pragma once

#include <vector>
#include <PhxResource/IResource.h>
#include <hlsl++.h>

#include "SceneComponents.h"
namespace phx
{
    struct Transform
    {
        hlslpp::float3 translation = { 0.0f, 0.0f, 0.0f };
        hlslpp::float4 rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
        hlslpp::float3 scale = { 0.0f, 0.0f, 0.0f };
    };


    using NodeHandle = int32_t;
	struct SceneNode
	{
        std::string name;
        Transform local_transform;

        // A list of components that define what this node is.
        std::vector<std::unique_ptr<scene::Component>> components;

        // Hierarchy information (remains the same)
        NodeHandle parent_index = -1;
        std::vector<int> children_indices;

        template<typename T>
        T& EmplaceComponent()
        {
            auto comp = std::make_unique<T>();
            components.emplace_back(std::move(comp));

            return *components.back();
        }

        // Helper function to get a specific component type
        template<typename T>
        T* GetComponent() const
        {
            constexpr uint64_t targetHash = T::StaticTypeHash();
            for (const auto& comp : components)
            {
                if (comp->type_hash == targetHash)
                {
                    return static_cat<T*>(comp);
                }
            }
            return nullptr;
        }
	};

	struct SceneBlueprint : public Resource
	{
        std::vector<int> root_node_indices;
        std::vector<SceneNode> nodes;

        SceneBlueprint() = default;

        NodeHandle AddNode(SceneNode&& node)
        {
            int index = static_cast<int>(nodes.size());
            nodes.push_back(std::move(node));
            return index;
        }

        const SceneNode* GetNode(NodeHandle index) const
        {
            if (index >= 0 && index < static_cast<int>(nodes.size()))
            {
                return &nodes[index];
            }

            return nullptr;
        }
	};
}