#pragma once

#include <vector>
#include <memory>
#include <string>

#include "Entity.h"

#include <PhxData/Asset.h>

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
   
namespace phx
{
    struct Transform
    {
        hlslpp::float3 translation = { 0.0f, 0.0f, 0.0f };
        hlslpp::float4 rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
        hlslpp::float3 scale = { 0.0f, 0.0f, 0.0f };
    };

    namespace scene
    {
        struct Component
        {
            const StringHash type_hash;

            virtual ~Component() = default;

        protected:
            explicit Component(uint64_t hash) : type_hash(hash) {}
        };

        struct MaterialAssignment
        {
            std::string material_virutal_path;
            uint32_t geometry_index = 0;
        };

        struct MeshComponent : public Component
        {
            std::string mesh_virtual_path;
            std::vector<MaterialAssignment> mat_assignments;

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

        struct CameraComponent : public Component
        {
            float field_of_view = 70.0f;

            PHX_DECLARE_COMPONENT(CameraComponent, Component)
        };
    }

    using SceneNodeHandle = size_t;
    constexpr SceneNodeHandle kInvalidSceneHandle = 0x7FFFFFFFFFFFFF;
	struct SceneNode
	{
        std::string name;
        Transform local_transform;

        // A list of components that define what this node is.
        std::vector<std::unique_ptr<scene::Component>> components;

        // Hierarchy information (remains the same)
        SceneNodeHandle parent_index = kInvalidSceneHandle;
        std::vector<SceneNodeHandle> children_indices;

        entt::entity runtime_entity;

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

    // TODO: this needs to be reworked.
	struct SceneBlueprint
	{
        std::vector<SceneNodeHandle> root_node_indices;
        std::vector<SceneNode> nodes;

        SceneNodeHandle AddNode(SceneNode&& node)
        {
            int index = static_cast<int>(nodes.size());
            nodes.push_back(std::move(node));
            return index;
        }

        const SceneNode* GetNode(SceneNodeHandle index) const
        {
            if (index >= 0 && index < nodes.size())
            {
                return &nodes[index];
            }

            return nullptr;
        }

        SceneNode* GetNode(SceneNodeHandle index)
        {
            if (index >= 0 && index < nodes.size())
            {
                return &nodes[index];
            }

            return nullptr;
        }
	};

}