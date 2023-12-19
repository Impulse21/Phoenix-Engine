#pragma once

#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Engine/Components.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Assets/AssetRegistry.h>

#include <DirectXMath.h>
#include <memory>
#include <entt.hpp>

namespace PhxEngine::World
{
	class Entity;

	class World
	{
		friend Entity;
	public:
		World() = default;
		~World() = default; 

	public:

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

		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

	private:
		entt::registry m_registry;
	};

	class IWorldLoader
	{
	public:
		virtual ~IWorldLoader() = default;
		
		virtual bool LoadWorld(std::string const& filename, Core::IFileSystem* fileSystem, Assets::AssetsRegistry& assetRegistry, World& outWorld) = 0;
	};

	class WorldLoaderFactory
	{
	public:
		static std::unique_ptr<IWorldLoader> Create();
	};
}
