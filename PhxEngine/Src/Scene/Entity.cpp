#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include "PhxEngine/Scene/Entity.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace PhxEngine::Scene;

Entity::Entity(entt::entity handle, New::Scene* scene)
	: m_entityHandle(handle)
	, m_scene(scene)
{
}

void Entity::AttachToParent(Entity parent, bool childInLocalSpace)
{
	assert(*this != parent);

	// Detatch so we can ensure that the child comes after the parent in the compoent Pool
	if (this->HasComponent<New::HierarchyComponent>())
	{
		this->DetachFromParent();
	}

	auto& comp = this->AddComponent<New::HierarchyComponent>();
	comp.ParentID = (entt::entity)parent;

	assert(parent.HasComponent<New::TransformComponent>());
	assert(this->HasComponent<New::TransformComponent>());

	auto& transformChild = this->GetComponent<New::TransformComponent>();
	auto& transformParent = parent.GetComponent<New::TransformComponent>();
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
	if (!this->HasComponent<New::HierarchyComponent>())
	{
		return;
	}

	assert(this->HasComponent<TransformComponent>());
	auto& transform = this->GetComponent<TransformComponent>();
	transform.ApplyTransform();

	this->RemoveComponent<HierarchyComponent>();
}

void Entity::DetachChildren()
{
	auto view = this->m_scene->GetAllEntitiesWith<New::HierarchyComponent>();
	for (auto entity : view)
	{
		auto hComp = view.get<New::HierarchyComponent>(entity);
		if (hComp.ParentID == this->m_entityHandle)
		{
			this->m_scene->GetRegistry().remove<New::HierarchyComponent>(entity);
		}
	}
}