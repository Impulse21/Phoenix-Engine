#include <PhxEngine/World/World.h>

#include <PhxEngine/World/Entity.h>

using namespace PhxEngine;

Entity PhxEngine::World::CreateEntity(std::string const& name)
{
	return this->CreateEntity(UUID(), name);
}

Entity PhxEngine::World::CreateEntity(UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}
