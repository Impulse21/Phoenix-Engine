#include "phxpch.h"

#include "PhxEngine/Scene/Scene.h"
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>

#include <DirectXMath.h>

#include <algorithm>


using namespace PhxEngine;
using namespace PhxEngine::Scene;
using namespace PhxEngine::RHI;
using namespace DirectX;

Entity PhxEngine::Scene::Scene::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

Entity PhxEngine::Scene::Scene::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void PhxEngine::Scene::Scene::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}

void PhxEngine::Scene::Scene::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void PhxEngine::Scene::Scene::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void PhxEngine::Scene::Scene::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}

void PhxEngine::Scene::Scene::ConstructRenderData(RHI::CommandListHandle cmd)
{
	auto view = this->GetAllEntitiesWith<MeshRenderComponent>();
	for (auto e : view)
	{
		auto comp = view.get<MeshRenderComponent>(e);
		comp.Mesh->CreateRenderData(cmd);
	}
}

RHI::DescriptorIndex PhxEngine::Scene::Scene::GetBrdfLutDescriptorIndex()
{
	if (!this->m_brdfLut)
	{
		return RHI::cInvalidDescriptorIndex;
	}

	return IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_brdfLut->GetRenderHandle(), RHI::SubresouceType::SRV);
}
