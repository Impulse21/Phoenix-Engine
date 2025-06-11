#pragma once

#include <vector>


#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>
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

	struct SceneBlueprint : public phx::data::Asset
	{
        std::vector<int> root_node_indices;
        std::vector<SceneNode> nodes;

        PHX_DECLARE_ASSET(SceneBlueprint)

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