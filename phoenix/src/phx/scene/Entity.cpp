#include "phxpch.h"
#include "Entity.h"
#include "components/CoreComponents.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace phx;

Entity::Entity(entt::entity handle, Scene* scene)
	: m_entityHandle(handle)
	, m_scene(scene)
{
}

void Entity::AttachToParent(Entity parent, bool childInLocalSpace)
{
	PHX_ASSERT(*this != parent);

	// Detatch so we can ensure that the child comes after the parent in the compoent Pool
	if (this->HasComponent<HierarchyComponent>())
	{
		this->DetachFromParent();
	}

	auto& comp = this->AddComponent<HierarchyComponent>();
	comp.ParentID = (entt::entity)parent;

	PHX_ASSERT(parent.HasComponent<TransformComponent>());
	PHX_ASSERT(this->HasComponent<TransformComponent>());

	auto& transformChild = this->GetComponent<TransformComponent>();
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
	if (!this->HasComponent<HierarchyComponent>())
	{
		return;
	}

	PHX_ASSERT(this->HasComponent<TransformComponent>());
	auto& transform = this->GetComponent<TransformComponent>();
	transform.ApplyTransform();

	this->RemoveComponent<HierarchyComponent>();
}

void Entity::DetachChildren()
{
	auto view = this->m_scene->GetAllEntitiesWith<HierarchyComponent>();
	for (auto entity : view)
	{
		auto hComp = view.get<HierarchyComponent>(entity);
		if (hComp.ParentID == this->m_entityHandle)
		{
			this->m_scene->GetRegistry().remove<HierarchyComponent>(entity);
		}
	}
}