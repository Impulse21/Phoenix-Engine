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

void PhxEngine::Renderer::MeshComponent::CreateRenderData(
	RHI::IGraphicsDevice* graphicsDevice,
	RHI::CommandListHandle commandList)
{
	// Construct the Mesh buffer
	if (!this->Indices.empty() && !this->IndexGpuBuffer)
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = sizeof(uint32_t) * this->Indices.size();
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexGpuBuffer = graphicsDevice->CreateIndexBuffer(indexBufferDesc);

		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer<uint32_t>(this->IndexGpuBuffer, this->Indices);
		commandList->TransitionBarrier(
			this->IndexGpuBuffer,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource);
	}

	// Construct the Vertex Buffer
	// Set up the strides and offsets
	if (!this->VertexGpuBuffer)
	{
		RHI::BufferDesc vertexDesc = {};
		vertexDesc.StrideInBytes = sizeof(float);
		vertexDesc.DebugName = "Vertex Buffer";
		vertexDesc.EnableBindless();
		vertexDesc.IsRawBuffer();
		vertexDesc.CreateSrvViews();

		const uint64_t alignment = 16ull;
		vertexDesc.SizeInBytes =
			AlignTo(this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3), alignment) +
			AlignTo(this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2), alignment) +
			AlignTo(this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3), alignment) + 
			AlignTo(this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4), alignment) +
			AlignTo(this->VertexColour.size() * sizeof(DirectX::XMFLOAT3), alignment);

		// Is this Needed for Raw Buffer Type
		this->VertexGpuBuffer = graphicsDevice->CreateVertexBuffer(vertexDesc);

		std::vector<uint8_t> gpuBufferData(vertexDesc.SizeInBytes);
		std::memset(gpuBufferData.data(), 0, vertexDesc.SizeInBytes);
		uint64_t bufferOffset = 0ull;

		auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* data, uint64_t sizeInBytes)
		{
			auto& bufferRange = this->GetVertexAttribute(attr);
			bufferRange.ByteOffset = bufferOffset;
			bufferRange.SizeInBytes = sizeInBytes;

			bufferOffset += AlignTo(bufferRange.SizeInBytes, alignment);
			// DirectX::XMFLOAT3* vertices = reinterpret_cast<DirectX::XMFLOAT3*>(gpuBufferData.data() + bufferOffset);
			std::memcpy(gpuBufferData.data() + bufferRange.ByteOffset, data, bufferRange.SizeInBytes);
		};

		if (!this->VertexPositions.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Position,
				this->VertexPositions.data(),
				this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3));
		}

		if (!this->VertexTexCoords.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::TexCoord,
				this->VertexTexCoords.data(),
				this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2));
		}

		if (!this->VertexNormals.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Normal,
				this->VertexNormals.data(),
				this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3));
		}

		if (!this->VertexTangents.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Tangent,
				this->VertexTangents.data(),
				this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4));
		}

		if (!this->VertexColour.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Colour,
				this->VertexColour.data(),
				this->VertexColour.size() * sizeof(DirectX::XMFLOAT3));
		}

		// Write Data
		commandList->TransitionBarrier(this->VertexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(this->VertexGpuBuffer, gpuBufferData, 0);
		commandList->TransitionBarrier(this->VertexGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
	}
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

void PhxEngine::Renderer::CameraComponent::TransformCamera(TransformComponent const& transform)
{
	XMVECTOR scale, rotation, translation;
	XMMatrixDecompose(
		&scale,
		&rotation,
		&translation, 
		XMLoadFloat4x4(&transform.WorldMatrix));

	XMVECTOR eye = translation;
	XMVECTOR at = XMVectorSet(0, 0, 1, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	XMMATRIX rot = XMMatrixRotationQuaternion(rotation);
	at = XMVector3TransformNormal(at, rot);
	up = XMVector3TransformNormal(up, rot);
	// XMStoreFloat3x3(&rotationMatrix, _Rot);

	XMStoreFloat3(&this->Eye, eye);
	XMStoreFloat3(&this->At, at);
	XMStoreFloat3(&this->Up, up);
}

void PhxEngine::Renderer::CameraComponent::UpdateCamera()
{
	XMVECTOR cameraPos = XMLoadFloat3(&this->Eye);
	XMVECTOR cameraLookAt = XMLoadFloat3(&this->At);
	XMVECTOR cameraUp = XMLoadFloat3(&this->Up);
	
	auto viewMatrix = this->ConstructViewMatrixLH();

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

DirectX::XMMATRIX PhxEngine::Renderer::CameraComponent::ConstructViewMatrixLH()
{
	const XMVECTOR forward = XMLoadFloat3(&this->At);
	const XMVECTOR axisZ = XMVector3Normalize(forward);

	// axisX == right vector
	const XMVECTOR up = XMLoadFloat3(&this->Up);
	const XMVECTOR axisX = XMVector3Normalize(XMVector3Cross(up, forward));

	// Axisy == Up vector ( forward cross with right)
	const XMVECTOR axisY = XMVector3Cross(axisZ, axisX);

	// --- Construct View matrix ---
	const XMVECTOR negEye = XMVectorNegate(XMLoadFloat3(&this->Eye));

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

void PhxEngine::Renderer::MaterialComponent::PopulateShaderData(Shader::MaterialData& shaderData)
{
	shaderData.AlbedoColour = { this->Albedo.x, this->Albedo.y, this->Albedo.z };
	shaderData.AO = this->Ao;

	shaderData.AlbedoTexture = INVALID_DESCRIPTOR_INDEX;
	if (this->AlbedoTexture)
	{
		shaderData.AlbedoTexture = this->AlbedoTexture->GetDescriptorIndex();
	}

	shaderData.AOTexture = INVALID_DESCRIPTOR_INDEX;
	if (this->AoTexture)
	{
		shaderData.AOTexture = this->AoTexture->GetDescriptorIndex();
	}

	shaderData.MaterialTexture = INVALID_DESCRIPTOR_INDEX;
	if (this->MetalRoughnessTexture)
	{
		shaderData.MaterialTexture = this->MetalRoughnessTexture->GetDescriptorIndex();
	}

	shaderData.NormalTexture = INVALID_DESCRIPTOR_INDEX;
	if (this->NormalMapTexture)
	{
		shaderData.NormalTexture = this->NormalMapTexture->GetDescriptorIndex();
	}
}
