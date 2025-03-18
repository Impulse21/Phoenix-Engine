#pragma once

#include <PhxCore/UUID.h>
#include <entt/entt.hpp>

#include "World.h"
#include "WorldComponents.h"

namespace phx
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, World* scene);
		Entity(Entity const& other) = default;

		// Wrappers
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			assert(!HasComponent<T>());
			T& component = m_world->GetRegistry().emplace<T>(m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_world->GetRegistry().emplace_or_replace<T>(m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			assert(HasComponent<T>());
			return m_world->GetRegistry().get<T>(m_entityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_world->GetRegistry().all_of<T>(m_entityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			assert(HasComponent<T>());

			m_world->GetRegistry().remove<T>(m_entityHandle);
		}

		void AttachToParent(Entity parent, bool childInLocalSpace = false);
		void DetachFromParent();
		void DetachChildren();

		operator bool() const { return m_entityHandle != entt::null; }
		operator entt::entity() const { return m_entityHandle; }
		operator uint32_t() const { return (uint32_t)m_entityHandle; }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		const std::string& GetName() { return GetComponent<NameComponent>().Name; }

		bool operator==(const Entity& other) const
		{
			return m_entityHandle == other.m_entityHandle && m_world == other.m_world;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_entityHandle = entt::null;
		World* m_world;
	};
}