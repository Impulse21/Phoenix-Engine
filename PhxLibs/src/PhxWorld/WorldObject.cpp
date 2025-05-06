#include "PhxWorld_pch.h"

#include "WorldObject.h"

#include "WorldComponents.h"

phx::WorldObject::WorldObject(Entity entity)
	: m_entity(entity)
{
    auto& tc = m_entity.AddComponent<TransformComponent>();

	DirectX::XMVECTOR scale = DirectX::XMLoadFloat3(&m_scale);
	DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&m_rotation);
	DirectX::XMVECTOR translation = DirectX::XMLoadFloat3(&m_translation);
	DirectX::XMMATRIX m = 
		DirectX::XMMatrixScalingFromVector(scale) *
		DirectX::XMMatrixRotationQuaternion(rotation) *
		DirectX::XMMatrixTranslationFromVector(translation);

	DirectX::XMStoreFloat4x4(&tc.WorldMatrix, m);

	if (m_parent)
	{
		auto& hc = m_entity.AddComponent<HierarchyComponent>();
		hc.ParentID = m_parent->GetParent()->GetEntity();
	}
}
