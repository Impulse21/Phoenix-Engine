#include <PhxEngine/Engine/Entity.h>

using namespace PhxEngine;

World::Entity::Entity(entt::entity handle, World* scene)
	: m_entityHandle(handle)
	, m_world(scene)
{
}

void World::Entity::AttachToParent(Entity parent, bool childInLocalSpace)
{
	assert(*this != parent);

	// Detatch so we can ensure that the child comes after the parent in the compoent Pool
	if (this->HasComponent<HierarchyComponent>())
	{
		this->DetachFromParent();
	}

	auto& comp = this->AddComponent<HierarchyComponent>();
	comp.ParentID = (entt::entity)parent;

	assert(parent.HasComponent<TransformComponent>());
	assert(this->HasComponent<TransformComponent>());

	auto& transformChild = this->GetComponent<TransformComponent>();
	auto& transformParent = parent.GetComponent<TransformComponent>();
	if (!childInLocalSpace)
	{
		DirectX::XMMATRIX B = XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&transformParent.WorldMatrix));
		transformChild.MatrixTransform(B);
		transformChild.UpdateTransform();
	}

	transformChild.UpdateTransform(transformParent);
}

void World::Entity::DetachFromParent()
{
	if (!this->HasComponent<HierarchyComponent>())
	{
		return;
	}

	assert(this->HasComponent<TransformComponent>());
	auto& transform = this->GetComponent<TransformComponent>();
	transform.ApplyTransform();

	this->RemoveComponent<HierarchyComponent>();
}

void World::Entity::DetachChildren()
{
	auto view = this->m_world->GetAllEntitiesWith<HierarchyComponent>();
	for (auto entity : view)
	{
		auto hComp = view.get<HierarchyComponent>(entity);
		if (hComp.ParentID == this->m_entityHandle)
		{
			this->m_world->GetRegistry().remove<HierarchyComponent>(entity);
		}
	}
}