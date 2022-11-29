#pragma once

#include <sstream>

#include "PhxEngine/Core/UUID.h"
#include "PhxEngine/Graphics/RHI/PhxRHI.h"
#include "PhxEngine/Scene/Assets.h"

#include <entt.hpp>

namespace PhxEngine::Scene
{
	class Entity;
	class ISceneWriter;

	class Scene
	{
		friend Entity;
		friend ISceneWriter;
	public:
		~Scene()
		{
			this->m_registry.clear();
		};

		Entity CreateEntity(std::string const& name = std::string());
		Entity CreateEntity(Core::UUID uuid, std::string const& name = std::string());

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

		void ConstructRenderData(RHI::CommandListHandle cmd);

		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

		void SetBrdfLut(std::shared_ptr<Assets::Texture> texture) { this->m_brdfLut = texture; }
		RHI::DescriptorIndex GetBrdfLutDescriptorIndex();

	private:
		entt::registry m_registry;
		std::shared_ptr<Assets::Texture> m_brdfLut;
	};

}