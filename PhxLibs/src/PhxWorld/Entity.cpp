#include "PhxWorld_pch.h"

#include "Entity.h"
#include <DirectXMath.h>
#include <PhxData/WorldComponents.def.h>

using namespace DirectX;
using namespace phx;

Entity::Entity(entt::entity handle, World* scene)
	: m_entityHandle(handle)
	, m_world(scene)
{
}

void Entity::AttachToParent(Entity parent, bool childInLocalSpace)
{
	assert(*this != parent);

	// Detatch so we can ensure that the child comes after the parent in the compoent Pool
	if (HasComponent<HierarchyComponent>())
	{
		DetachFromParent();
	}

	auto& comp = AddComponent<HierarchyComponent>();
	comp.ParentID = (entt::entity)parent;

	assert(parent.HasComponent<TransformComponent>());
	assert(HasComponent<TransformComponent>());

	auto& transformChild = GetComponent<TransformComponent>();
	auto& transformParent = parent.GetComponent<TransformComponent>();
	if (!childInLocalSpace)
	{
		XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transformParent.WorldMatrix));
		transformChild.MatrixTransform(B);
		transformChild.UpdateTransform();
	}

	transformChild.UpdateTransform(transformParent);
}

void Entity::DetachFromParent()
{
	if (!HasComponent<HierarchyComponent>())
	{
		return;
	}

	assert(HasComponent<TransformComponent>());
	auto& transform = GetComponent<TransformComponent>();
	transform.ApplyTransform();

	RemoveComponent<HierarchyComponent>();
}

void Entity::DetachChildren()
{
	auto view = m_world->GetAllEntitiesWith<HierarchyComponent>();
	for (auto entity : view)
	{
		auto hComp = view.get<HierarchyComponent>(entity);
		if (hComp.ParentID == m_entityHandle)
		{
			m_world->GetRegistry().remove<HierarchyComponent>(entity);
		}
	}
}