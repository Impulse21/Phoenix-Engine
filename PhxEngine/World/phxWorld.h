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

	public:
		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

	private:
		entt::registry m_registry;
	};
}

