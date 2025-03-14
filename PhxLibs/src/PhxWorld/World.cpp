#include "PhxWorld_pch.h"

#include "Entity.h"

#include <DirectXMath.h>

#include <algorithm>

using namespace phx;
using namespace DirectX;

constexpr uint64_t kVertexBufferAlignment = 16ull;


// #define ENABLE_ALIASED_BUFFER
World::World() 

Entity World::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

Entity World::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void World::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}

void World::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void World::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void World::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}
