#include <PhxEngine/Renderer/SceneComponents.h>

using namespace PhxEngine::Renderer;
using namespace PhxEngine::ECS;
using namespace DirectX;

DirectX::XMFLOAT3 PhxEngine::Renderer::TransformComponent::GetPosition() const
{
	return *((XMFLOAT3*)&this->WorldMatrix._41);
}

DirectX::XMFLOAT4 PhxEngine::Renderer::TransformComponent::GetRotation() const
{
	XMFLOAT4 rotation;
	XMStoreFloat4(&rotation, this->GetRotationV());
	return rotation;
}

DirectX::XMFLOAT3 PhxEngine::Renderer::TransformComponent::GetScale() const
{
	XMFLOAT3 scale;
	XMStoreFloat3(&scale, this->GetScaleV());
	return scale;
}

DirectX::XMVECTOR PhxEngine::Renderer::TransformComponent::GetPositionV() const
{
	return XMLoadFloat3((XMFLOAT3*)&this->WorldMatrix._41);
}

DirectX::XMVECTOR PhxEngine::Renderer::TransformComponent::GetRotationV() const
{
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&this->WorldMatrix));
	return R;
}

DirectX::XMVECTOR PhxEngine::Renderer::TransformComponent::GetScaleV() const
{
	XMVECTOR S, R, T;
	XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&this->WorldMatrix));
	return S;
}

XMMATRIX PhxEngine::Renderer::TransformComponent::GetLocalMatrix() const
{
	XMVECTOR localScale = XMLoadFloat3(&this->LocalScale);
	XMVECTOR localRotation= XMLoadFloat4(&this->LocalRotation);
	XMVECTOR localTranslation = XMLoadFloat3(&this->LocalTranslation);
	return
		XMMatrixScalingFromVector(localScale) *
		XMMatrixRotationQuaternion(localRotation) *
		XMMatrixTranslationFromVector(localTranslation);
}

void PhxEngine::Renderer::TransformComponent::UpdateTransform()
{
	if (this->IsDirty())
	{
		this->SetDirty(false);

		XMStoreFloat4x4(&this->WorldMatrix, this->GetLocalMatrix());
	}
}

void PhxEngine::Renderer::TransformComponent::UpdateTransform(TransformComponent const& parent)
{
	XMMATRIX world = this->GetLocalMatrix();
	XMMATRIX worldParentworldParent = XMLoadFloat4x4(&parent.WorldMatrix);
	world *= worldParentworldParent;

	XMStoreFloat4x4(&WorldMatrix, world);
}

void PhxEngine::Renderer::TransformComponent::ApplyTransform()
{
	this->SetDirty();

	XMVECTOR scalar, rotation, translation;
	XMMatrixDecompose(&scalar, &rotation, &translation, XMLoadFloat4x4(&this->WorldMatrix));
	XMStoreFloat3(&this->LocalScale, scalar);
	XMStoreFloat4(&this->LocalRotation, rotation);
	XMStoreFloat3(&this->LocalTranslation, translation);
}

void PhxEngine::Renderer::TransformComponent::ClearTransform()
{
	this->SetDirty();
	this->LocalScale = XMFLOAT3(1, 1, 1);
	this->LocalRotation = XMFLOAT4(0, 0, 0, 1);
	this->LocalTranslation = XMFLOAT3(0, 0, 0);
}

void PhxEngine::Renderer::TransformComponent::Translate(const XMFLOAT3& value)
{
	this->SetDirty();
	this->LocalTranslation.x += value.x;
	this->LocalTranslation.y += value.y;
	this->LocalTranslation.z += value.z;
}

void PhxEngine::Renderer::TransformComponent::Translate(DirectX::XMVECTOR const& value)
{
	XMFLOAT3 translation;
	XMStoreFloat3(&translation, value);
	this->Translate(translation);
}

void TransformComponent::RotateRollPitchYaw(const XMFLOAT3& value)
{
	this->SetDirty();

	// This needs to be handled a bit differently
	XMVECTOR quat = XMLoadFloat4(&this->LocalRotation);
	XMVECTOR x = XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
	XMVECTOR y = XMQuaternionRotationRollPitchYaw(0, value.y, 0);
	XMVECTOR z = XMQuaternionRotationRollPitchYaw(0, 0, value.z);

	quat = XMQuaternionMultiply(x, quat);
	quat = XMQuaternionMultiply(quat, y);
	quat = XMQuaternionMultiply(z, quat);
	quat = XMQuaternionNormalize(quat);

	XMStoreFloat4(&this->LocalRotation, quat);
}
void TransformComponent::Rotate(const XMFLOAT4& quaternion)
{
	this->SetDirty();

	XMVECTOR result = XMQuaternionMultiply(XMLoadFloat4(&this->LocalRotation), XMLoadFloat4(&quaternion));
	result = XMQuaternionNormalize(result);
	XMStoreFloat4(&this->LocalRotation, result);
}
void TransformComponent::Rotate(const XMVECTOR& quaternion)
{
	XMFLOAT4 rotation;
	XMStoreFloat4(&rotation, quaternion);
	Rotate(rotation);
}
void TransformComponent::Scale(const XMFLOAT3& value)
{
	this->SetDirty();
	this->LocalScale.x *= value.x;
	this->LocalScale.y *= value.y;
	this->LocalScale.z *= value.z;
}
void TransformComponent::Scale(const XMVECTOR& value)
{
	XMFLOAT3 scale;
	XMStoreFloat3(&scale, value);
	Scale(scale);
}
void TransformComponent::MatrixTransform(const XMFLOAT4X4& matrix)
{
	MatrixTransform(XMLoadFloat4x4(&matrix));
}
void TransformComponent::MatrixTransform(const XMMATRIX& matrix)
{
	this->SetDirty();

	XMVECTOR scale;
	XMVECTOR rotate;
	XMVECTOR translate;
	XMMatrixDecompose(&scale, &rotate, &translate, this->GetLocalMatrix() * matrix);

	XMStoreFloat3(&this->LocalScale, scale);
	XMStoreFloat4(&this->LocalRotation, rotate);
	XMStoreFloat3(&this->LocalTranslation, translate);
}

void PhxEngine::Renderer::MeshComponent::CreateRenderData(RHI::IGraphicsDevice* graphicsDevice, RHI::CommandListHandle commandList)
{
	auto CreateAndUploadVertexBuffer = [&](
		RHI::BufferHandle& buffer,
		const void* data,
		size_t numElements,
		size_t stride,
		std::string const& debugName,
		RHI::ResourceStates finalState,
		bool isBindless = true)
	{
		RHI::BufferDesc desc = {};
		desc.SizeInBytes = numElements * stride;
		desc.StrideInBytes = stride;
		desc.DebugName = debugName;
		desc.CreateBindless = isBindless;
		desc.CreateSRVViews = isBindless;

		buffer = graphicsDevice->CreateVertexBuffer(desc);
		commandList->TransitionBarrier(buffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(buffer, data, stride * numElements);
		commandList->TransitionBarrier(buffer, RHI::ResourceStates::CopyDest,finalState);
	};

	if (!this->IndexGpuBuffer && !this->Indices.empty())
	{
		RHI::BufferDesc desc = {};
		desc.SizeInBytes = this->Indices.size() * sizeof(this->Indices[0]);
		desc.StrideInBytes = sizeof(this->Indices[0]);
		desc.DebugName = "IndexBuffer";

		this->IndexGpuBuffer = graphicsDevice->CreateIndexBuffer(desc);
		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(this->IndexGpuBuffer, this->Indices.data(), desc.SizeInBytes);
		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::IndexGpuBuffer);
	}

	if (!this->VertexGpuBufferPositions && !this->VertexPositions.empty())
	{
		CreateAndUploadVertexBuffer(
			this->VertexGpuBufferPositions,
			this->VertexPositions.data(),
			this->VertexPositions.size(),
			sizeof(this->VertexPositions[0]),
			"Vertex Position Buffer",
			RHI::ResourceStates::ShaderResource | RHI::ResourceStates::VertexBuffer);
	}

	if (!this->VertexGpuBufferTexCoords && !this->VertexTexCoords.empty())
	{
		CreateAndUploadVertexBuffer(
			this->VertexGpuBufferTexCoords,
			this->VertexTexCoords.data(),
			this->VertexTexCoords.size(),
			sizeof(this->VertexTexCoords[0]),
			"Vertex Tex Coords Buffer",
			RHI::ResourceStates::ShaderResource | RHI::ResourceStates::VertexBuffer);
	}

	if (!this->VertexGpuBufferNormals && !this->VertexNormals.empty())
	{
		CreateAndUploadVertexBuffer(
			this->VertexGpuBufferNormals,
			this->VertexNormals.data(),
			this->VertexNormals.size(),
			sizeof(this->VertexNormals[0]),
			"Vertex Normal Buffer",
			RHI::ResourceStates::ShaderResource | RHI::ResourceStates::VertexBuffer);
	}

	if (!this->VertexGpuBufferTangents && !this->VertexTangents.empty())
	{
		CreateAndUploadVertexBuffer(
			this->VertexGpuBufferTangents,
			this->VertexTangents.data(),
			this->VertexTangents.size(),
			sizeof(this->VertexTangents[0]),
			"Vertex Tangent Buffer",
			RHI::ResourceStates::ShaderResource | RHI::ResourceStates::VertexBuffer);
	}
}

void PhxEngine::Renderer::MeshComponent::PopulateShaderMeshData(Shader::Mesh& mesh)
{
	mesh.Flags = this->Flags;
	mesh.VbPositionBufferIndex = this->VertexGpuBufferPositions ? this->VertexGpuBufferPositions->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mesh.VbNormalBufferIndex = this->VertexGpuBufferNormals ? this->VertexGpuBufferNormals->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mesh.VbTexCoordBufferIndex = this->VertexGpuBufferTexCoords ? this->VertexGpuBufferTexCoords->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mesh.VbTangentBufferIndex = this->VertexGpuBufferTangents ? this->VertexGpuBufferTangents->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
}

void PhxEngine::Renderer::MeshComponent::ReverseWinding()
{
	PHX_ASSERT(this->IsTriMesh());
	for (auto iter = this->Indices.begin(); iter != this->Indices.end(); iter += 3)
	{
		std::swap(*iter, *(iter + 2));
	}
}

void PhxEngine::Renderer::MeshComponent::ComputeTangentSpace()
{
	// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#tangent-and-bitangent

	PHX_ASSERT(this->IsTriMesh());
	PHX_ASSERT(!this->VertexPositions.empty());

	const size_t vertexCount = this->VertexPositions.size();
	this->VertexTangents.resize(vertexCount);

	std::vector<XMVECTOR> tangents(vertexCount);
	std::vector<XMVECTOR> bitangents(vertexCount);

	for (int i = 0; i < this->Indices.size(); i += 3)
	{
		auto& index0 = this->Indices[i + 0];
		auto& index1 = this->Indices[i + 1];
		auto& index2 = this->Indices[i + 2];

		// Vertices
		XMVECTOR pos0 = XMLoadFloat3(&this->VertexPositions[index0]);
		XMVECTOR pos1 = XMLoadFloat3(&this->VertexPositions[index1]);
		XMVECTOR pos2 = XMLoadFloat3(&this->VertexPositions[index2]);

		// UVs
		XMVECTOR uvs0 = XMLoadFloat2(&this->VertexTexCoords[index0]);
		XMVECTOR uvs1 = XMLoadFloat2(&this->VertexTexCoords[index1]);
		XMVECTOR uvs2 = XMLoadFloat2(&this->VertexTexCoords[index2]);

		XMVECTOR deltaPos1 = pos1 - pos0;
		XMVECTOR deltaPos2 = pos2 - pos0;

		XMVECTOR deltaUV1 = uvs1 - uvs0;
		XMVECTOR deltaUV2 = uvs2 - uvs0;

		// TODO: Take advantage of SIMD better here
		float r = 1.0f / (XMVectorGetX(deltaUV1) * XMVectorGetY(deltaUV2) - XMVectorGetY(deltaUV1) * XMVectorGetX(deltaUV2));

		XMVECTOR tangent = (deltaPos1 * XMVectorGetY(deltaUV2) - deltaPos2 * XMVectorGetY(deltaUV1)) * r;
		XMVECTOR bitangent = (deltaPos2 * XMVectorGetX(deltaUV1) - deltaPos1 * XMVectorGetX(deltaUV2)) * r;

		tangents[index0] += tangent;
		tangents[index1] += tangent;
		tangents[index2] += tangent;

		bitangents[index0] += bitangent;
		bitangents[index1] += bitangent;
		bitangents[index2] += bitangent;
	}

	PHX_ASSERT(this->VertexNormals.size() == vertexCount);
	for (int i = 0; i < vertexCount; i++)
	{
		const XMVECTOR normal = XMLoadFloat3(&this->VertexNormals[i]);
		const XMVECTOR& tangent = tangents[i];
		const XMVECTOR& bitangent = bitangents[i];

		// Gram-Schmidt orthogonalize

		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		XMVectorSetW(tangent, sign);
		DirectX::XMStoreFloat4(&this->VertexTangents[i], orthTangent);
	}
}

void PhxEngine::Renderer::CameraComponent::UpdateCamera()
{
	XMVECTOR cameraPos = XMLoadFloat3(&this->Eye);
	XMVECTOR cameraLookAt = XMLoadFloat3(&this->At);
	XMVECTOR cameraUp = XMLoadFloat3(&this->Up);
	
	auto viewMatrix = XMMatrixLookAtLH(
		cameraPos,
		cameraLookAt,
		cameraUp);

	XMStoreFloat4x4(&this->View, viewMatrix);
	XMStoreFloat4x4(&this->ViewInv, XMMatrixInverse(nullptr, viewMatrix));

	auto projectionMatrix = XMMatrixPerspectiveFovLH(
		this->FoV,
		1.7f,
		this->ZNear,
		this->ZFar);

	XMStoreFloat4x4(&this->Projection, projectionMatrix);
	XMStoreFloat4x4(&this->ProjectionInv, XMMatrixInverse(nullptr, projectionMatrix));

	auto viewProjectionMatrix = viewMatrix * projectionMatrix;

	XMStoreFloat4x4(&this->ViewProjection, viewProjectionMatrix);

	XMStoreFloat4x4(&this->ViewProjectionInv, XMMatrixInverse(nullptr, viewProjectionMatrix));
}
