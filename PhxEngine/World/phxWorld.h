#pragma once

#include "entt/entt.hpp"

namespace phx
{
	class Entity;

	class World final
	{
		friend Entity;
	public:
		World() = default;
		~World() = default;

	public:
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return this->m_registry.view<Components...>();
		}

		template<typename... Components>
		auto GetAllEntitiesWith() const
		{
			return this->m_registry.view<Components...>();
		}


		Entity CreateEntity(std::string const& name = std::string());
		Entity CreateEntity(UUID uuid, std::string const& name = std::string());

		void DestroyEntity(Entity& entity);

		void AttachToParent(Entity& entity, Entity& parent, bool childInLocalSpace = false);
		void DetachFromParent(Entity& entity);
		void DetachChildren(Entity& parent);

	public:
		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

	private:
		entt::registry m_registry;
	};
}

