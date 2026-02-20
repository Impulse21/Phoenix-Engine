#include "PhxWorld_pch.h"

#include <PhxWorld/Entity.h>
#include <DirectXMath.h>
#include <PhxWorld/WorldComponents.h>

using namespace DirectX;
using namespace phx;

Entity::Entity(entt::entity handle, World* scene)
	: m_entityHandle(handle)
	, m_world(scene)
{
}

void Entity::AttachToParent(Entity parent, bool /*childInLocalSpace*/)
{
	assert(*this != parent);

	// Detatch so we can ensure that the child comes after the parent in the compoent Pool
	if (HasComponent<HierarchyComponent>())
	{
		DetachFromParent();
	}

	auto& comp = AddComponent<HierarchyComponent>();
	comp.ParentID = (entt::entity)parent;

	// TODO:
#if false
	assert(parent.HasComponent<TransformComponent>());
	assert(HasComponent<WorldTransformComponent>());

	auto& transformChild = GetComponent<TransformComponent>();
	auto& parent_world_transform = parent.GetComponent<WorldTransformComponent>();
	if (!childInLocalSpace)

		hlslpp::float4x4 inverse = hlslpp::inverse(parent_world_transform.world_matrix);
		
		// TODO: Construct matrix
		transformChild.MatrixTransform(B);
		transformChild.UpdateTransform();
	}
#endif
}

void Entity::DetachFromParent()
{
	if (!HasComponent<HierarchyComponent>())
	{
		return;
	}

	assert(HasComponent<TransformComponent>());

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