#pragma once

#include <entt/entt.hpp>
#include "phx/core/Math.h"
#include "phx/core/UUID.h"
namespace phx
{
	class Entity;
	class Scene
	{
		friend Entity;
	public:
		Scene();

		~Scene()
		{
			this->m_registry.clear();
		};

		void Initialize();

		Entity CreateEntity(std::string const& name = std::string());
		Entity CreateEntity(UUID uuid, std::string const& name = std::string());

		void DestroyEntity(Entity entity);

		void AttachToParent(Entity entity, Entity parent, bool childInLocalSpace = false);
		void DetachFromParent(Entity entity);
		void DetachChildren(Entity parent);

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


		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

	private:
		void OnConstructOrUpdate(entt::registry& registry, entt::entity entity);

	private:
		entt::registry m_registry;
		math::AABB m_sceneBounds;
	};
}