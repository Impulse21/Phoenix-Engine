#include "phxpch.h"

#include "Entity.h"
#include <DirectXMath.h>
#include <algorithm>

#include "components/CoreComponents.h"

using namespace phx;
using namespace DirectX;

Scene::Scene() = default;

void Scene::Initialize()
{
	/*
	this->GetRegistry().on_construct<MeshInstanceComponent>().connect<OnConstructOrUpdate>();
	this->GetRegistry().on_update<MeshInstanceComponent>().connect<OnConstructOrUpdate>();

	this->GetRegistry().on_construct<LightComponent>().connect<OnConstructOrUpdate>();
	this->GetRegistry().on_update<LightComponent>().connect<OnConstructOrUpdate>();
	*/
}

Entity Scene::CreateEntity(std::string const& name)
{
	return this->CreateEntity(UUID(), name);
}

Entity Scene::CreateEntity(UUID uuid, std::string const& name)
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
	PHX_ASSERT(entity != parent);
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


void Scene::OnConstructOrUpdate(entt::registry& registry, entt::entity entity)
{
	UNREFERENCED_PARAMETER(registry);
	UNREFERENCED_PARAMETER(entity);
}
