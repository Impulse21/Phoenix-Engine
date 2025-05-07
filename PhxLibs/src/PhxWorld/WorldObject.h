#pragma once

#include <memory>

#include <PhxData/DataContainers.h>
#include <PhxData/TypeReflection.h>
#include <entt/entt.hpp>

#include "Entity.h"
#include "WorldObjectComponent.h"

#include <DirectXMath.h>

namespace phx
{
	struct WorldObject
	{
        UUID ID;
        data::String Name = "";
        Entity Entity;

        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };

        WorldObject* Parent = nullptr;
        data::FlexArray<std::unique_ptr<WorldObject>> Children;

        ///data::FlexArray<std::unique_ptr<WorldObjectComponent>> ObjectComponents;

        template<typename T, typename... Args>
        T* AddObjectComponent(Args&&... args)
        {
            auto comp = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = comp.get();
            ObjectComponents.emplace_back(std::move(comp));
            return ptr;
        }

        template<typename T>
        T* GetObjectComponent()
        {


            for (auto& c : ObjectComponents)
            {
                if (c->IsInstanceOf<T>())
                    return c->As<T>();
            }

            return nullptr;
        }
	};

    REFLECT_BEGIN(WorldObject)
        REFLECT_FIELD(WorldObject, ID)
    REFLECT_END(WorldObject)
}
