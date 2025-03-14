#include "PhxScene_pch.h"

#include "Entity.h"

#include <DirectXMath.h>

#include <algorithm>

using namespace phx;
using namespace DirectX;

constexpr uint64_t kVertexBufferAlignment = 16ull;


// #define ENABLE_ALIASED_BUFFER
Scene::Scene() 

Entity Scene::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

Entity Scene::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void Scene::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}

void Scene::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void Scene::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void Scene::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}
