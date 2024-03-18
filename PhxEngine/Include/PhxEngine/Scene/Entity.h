#pragma once

#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <entt.hpp>

namespace PhxEngine
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(Entity const& other) = default;

		// Wrappers
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			assert(!this->HasComponent<T>());
			T& component = this->m_scene->m_registry.emplace<T>(this->m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = this->m_scene->m_registry.emplace_or_replace<T>(this->m_entityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			assert(this->HasComponent<T>());
			return this->m_scene->m_registry.get<T>(this->m_entityHandle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			assert(this->HasComponent<T>());
			return this->m_scene->m_registry.get<T>(this->m_entityHandle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return this->m_scene->m_registry.all_of<T>(this->m_entityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			assert(this->HasComponent<T>());

			this->m_scene->m_registry.remove<T>(this->m_entityHandle);
		}

		operator bool() const { return this->m_entityHandle != entt::null; }
		operator entt::entity() const { return this->m_entityHandle; }
		operator uint32_t() const { return (uint32_t)this->m_entityHandle; }

		UUID GetUUID() const { return this->GetComponent<IDComponent>().ID; }
		const std::string& GetName() const { return this->GetComponent<NameComponent>().Name; }

		bool operator==(const Entity& other) const
		{
			return this->m_entityHandle == other.m_entityHandle && this->m_scene == other.m_scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_entityHandle = entt::null;
		Scene* m_scene;
	};
}
