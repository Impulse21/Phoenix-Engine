#include <PhxEngine/Engine/World.h>

#include <PhxEngine/Engine/Entity.h>

#include <filesystem>

using namespace PhxEngine;

World::Entity World::World::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

World::Entity World::World::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void PhxEngine::World::World::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}


void PhxEngine::World::World::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void PhxEngine::World::World::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void PhxEngine::World::World::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}
