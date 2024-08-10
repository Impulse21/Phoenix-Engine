#pragma once

#include "phxWorld.h"
#include "phxComponents.h"

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
			assert(!this->HasComponent<T>());
			T& component = this->m_world->m_registry.emplace<T>(this->m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = this->m_world->m_registry.emplace_or_replace<T>(this->m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			assert(this->HasComponent<T>());
			return this->m_world->m_registry.get<T>(this->m_entityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return this->m_world->m_registry.all_of<T>(this->m_entityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			assert(this->HasComponent<T>());

			this->m_world->m_registry.remove<T>(this->m_entityHandle);
		}

		// Get Children
		std::vector<Entity> GetChildren();
		void AttachToParent(Entity parent, bool childInLocalSpace = false);
		void DetachFromParent();
		void DetachChildren();

		operator bool() const { return this->m_entityHandle != entt::null; }
		operator entt::entity() const { return this->m_entityHandle; }
		operator uint32_t() const { return (uint32_t)this->m_entityHandle; }

		UUID GetUUID() { return this->GetComponent<IDComponent>().ID; }
		const std::string& GetName() { return this->GetComponent<NameComponent>().Name; }

		bool operator==(const Entity& other) const
		{
			return this->m_entityHandle == other.m_entityHandle && this->m_world == other.m_world;
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

