#include <PhxEngine/Renderer/SceneNodes.h>

using namespace DirectX;
using namespace PhxEngine::Renderer;

void PhxEngine::Renderer::Node::SetTranslation(DirectX::XMVECTOR const& translation)
{
	DirectX::XMStoreFloat3(&this->m_translation, translation);
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

void Node::SetTranslation(DirectX::XMFLOAT3 const& translation)
{
	this->m_translation = translation;
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

void PhxEngine::Renderer::Node::SetRotation(DirectX::XMVECTOR const& rotationQuat)
{
	DirectX::XMStoreFloat4(&this->m_rotation, rotationQuat);
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

void PhxEngine::Renderer::Node::SetRotation(DirectX::XMFLOAT4 const& rotationQuat)
{
	this->m_rotation = rotationQuat;
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

void PhxEngine::Renderer::Node::SetScale(DirectX::XMVECTOR const& scale)
{
	DirectX::XMStoreFloat3(&this->m_scale, scale);
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

void Node::SetScale(DirectX::XMFLOAT3 const& scale)
{
	this->m_scale = scale;
	this->m_updateLocalTransform = true;
	this->InvalidateWorldMatrix();
}

const DirectX::XMMATRIX& PhxEngine::Renderer::Node::GetLocalTransform()
{
	if (this->m_updateLocalTransform)
	{
		const XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&this->m_rotation));
		const XMMATRIX scaleMatrix = XMMatrixScaling(this->m_scale.x, this->m_scale.y, this->m_scale.z);
		const XMMATRIX translationMatrix =
			XMMatrixTranslation(this->m_translation.x, this->m_translation.y, this->m_translation.z);

		this->m_localTransform = scaleMatrix * rotationMatrix * translationMatrix;
		this->m_updateLocalTransform = false;
	}

	return this->m_localTransform;
}

DirectX::XMMATRIX& PhxEngine::Renderer::Node::GetWorldMatrix()
{
	this->UpdateWorldMatrix();
	return this->m_worldMatrix;
}

void PhxEngine::Renderer::Node::InvalidateWorldMatrix()
{
	this->m_updateWorldMatrix = true;
}

void PhxEngine::Renderer::Node::SetParentNode(Node* node)
{
	this->m_parentNode = node;
	this->InvalidateWorldMatrix();
}

void PhxEngine::Renderer::Node::UpdateWorldMatrix()
{
	if (!this->m_updateWorldMatrix)
	{
		return;
	}

	this->m_worldMatrix = this->GetLocalTransform();
	auto parent = this->GetParentNode();

	if (parent)
	{
		this->m_worldMatrix = this->m_worldMatrix * parent->GetWorldMatrix();
	}

	this->m_updateWorldMatrix = false;
}

const DirectX::XMMATRIX& PhxEngine::Renderer::PerspectiveCameraNode::GetProjectionMatrix()
{
	if (this->m_isDirty)
	{
		this->m_projectionMatrix = XMMatrixPerspectiveFovLH(
			this->m_yFov,
			this->m_aspectRatio,
			this->m_zNear,
			this->m_zFar);
		this->m_isDirty = false;
	}

	return this->m_projectionMatrix;
	// TODO: insert return statement here
}


void PhxEngine::Renderer::PerspectiveCameraNode::UpdateViewMatrixLH()
{
	/*
	// axisZ == Forward Vector
	const XMVECTOR axisZ = XMVector3Normalize(forward);

	// axisX == right vector
	const XMVECTOR axisX = XMVector3Normalize(XMVector3Cross(up, forward));

	// Axisy == Up vector ( forward cross with right)
	const XMVECTOR axisY = XMVector3Cross(axisZ, axisX);

	this->m_viewMatrix = this->ConstructViewMatrixLH(
		axisX,
		axisY,
		axisZ,
		this->GetTranslation());
		*/
}

DirectX::XMMATRIX PhxEngine::Renderer::PerspectiveCameraNode::ConstructViewMatrixLH(
	DirectX::XMVECTOR const& axisX,
	DirectX::XMVECTOR const& axisY,
	DirectX::XMVECTOR const& axisZ,
	DirectX::XMVECTOR const& position) const
{
	const XMVECTOR negEye = XMVectorNegate(position);

	// Not sure I get this bit.
	const XMVECTOR d0 = XMVector3Dot(axisX, negEye);
	const XMVECTOR d1 = XMVector3Dot(axisY, negEye);
	const XMVECTOR d2 = XMVector3Dot(axisZ, negEye);

	// Construct column major view matrix;
	XMMATRIX m;
	m.r[0] = XMVectorSelect(d0, axisX, g_XMSelect1110.v);
	m.r[1] = XMVectorSelect(d1, axisY, g_XMSelect1110.v);
	m.r[2] = XMVectorSelect(d2, axisZ, g_XMSelect1110.v);
	m.r[3] = g_XMIdentityR3.v;

	return XMMatrixTranspose(m);
}

int PhxEngine::Renderer::SceneForwardIterator::Next(bool allowChildren)
{
	return 0;
}
