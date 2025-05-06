#pragma once

#include <memory>

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/Object.h>

#include <entt/entt.hpp>

#include "Entity.h"
#include "WorldObjectComponent.h"

#include <DirectXMath.h>

namespace phx
{
	class WorldObject : public RefCounter<Object>
	{
		PHX_OBJECT(WorldObject);

    public:
        WorldObject() = default;
        WorldObject(entt::entity handle, World* world)
            : WorldObject(Entity(handle, world)) 
        {
        };

        WorldObject(Entity entity);

        template<typename T, typename... Args>
        T* AddObjectComponent(Args&&... args)
        {
            auto comp = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = comp.get();
            m_objectComponents.emplace_back(std::move(comp));
            return ptr;
        }

        template<typename T>
        T* GetObjectComponent()
        {
            for (auto& c : m_objectComponents)
            {
                if (c->IsInstanceOf<T>())
                    return c->As<T>();
            }

            return nullptr;
        }

        WorldObject* GetParent() { return m_parent; }
        const Entity& GetEntity() const { return m_entity; }

    private:
        std::string m_name = "";
        Entity m_entity;

        DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 m_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 m_translation = { 0.0f, 0.0f, 0.0f };


        // Intrusive would have been better for performance. I think however
        // hard to work with the UI.
        WorldObject* m_parent = nullptr;
        std::vector<std::unique_ptr<WorldObject>> m_children;

        std::vector<std::unique_ptr<WorldObjectComponent>> m_objectComponents;

	};
}