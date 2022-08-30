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

Entity New::Scene::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

Entity New::Scene::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void New::Scene::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}

void New::Scene::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void New::Scene::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void New::Scene::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}

// --------------------------------------------------------------------------------
CameraComponent& Legacy::Scene::GetGlobalCamera()
{
	static CameraComponent mainCamera;
	return mainCamera;
}

ECS::Entity Legacy::Scene::EntityCreateMeshInstance(std::string const& name)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);

	MeshInstanceComponent& c = this->MeshInstances.Create(e);
	return e;
}

ECS::Entity Legacy::Scene::EntityCreateCamera(std::string const& name)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);

	CameraComponent& c = this->Cameras.Create(e);
	return e;
}

ECS::Entity Legacy::Scene::EntityCreateCamera(
	std::string const& name,
	float width,
	float height,
	float nearPlane,
	float farPlane,
	float fov)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);

	CameraComponent& c = this->Cameras.Create(e);
	c.Width = width;
	c.Height = height;
	c.ZNear = nearPlane;
	c.ZFar = farPlane;
	c.FoV = fov;

	return e;
}

ECS::Entity Legacy::Scene::EntityCreateMaterial(std::string const& name)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	MaterialComponent& c = this->Materials.Create(e);
	
	return e;
}

ECS::Entity Legacy::Scene::EntityCreateMesh(std::string const& name)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	MeshComponent& c = this->Meshes.Create(e);

	return e;
}

ECS::Entity Legacy::Scene::EntityCreateLight(std::string const& name)
{
	ECS::Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);
	this->Lights.Create(e);

	return e;
}

ECS::Entity Legacy::Scene::CreateCubeMeshEntity(std::string const& name, ECS::Entity mtlID, float size, bool rhsCoords)
{
	// A cube has six faces, each one pointing in a different direction.
	const int FaceCount = 6;

	static const XMFLOAT3 faceNormals[FaceCount] =
	{
		{ 0,  0,  1 },
		{ 0,  0, -1 },
		{ 1,  0,  0 },
		{ -1,  0,  0 },
		{ 0,  1,  0 },
		{ 0, -1,  0 },
	};

	static const XMFLOAT3 faceColour[] =
	{
		{ 1.0f,  0.0f,  0.0f },
		{ 0.0f,  1.0f,  0.0f },
		{ 0.0f,  0.0f,  1.0f },
	};

	static const XMFLOAT2 textureCoordinates[4] =
	{
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
	};

	ECS::Entity meshEntity = this->EntityCreateMesh(name);
	auto* meshComponent = this->Meshes.GetComponent(meshEntity);

	size /= 2;

	for (int i = 0; i < FaceCount; i++)
	{
		XMVECTOR normal = XMLoadFloat3(&faceNormals[i]);

		// Get two vectors perpendicular both to the face normal and to each other.
		XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

		XMVECTOR side1 = XMVector3Cross(normal, basis);
		XMVECTOR side2 = XMVector3Cross(normal, side1);

		// Six IndexData (two triangles) per face.
		size_t vbase = meshComponent->VertexPositions.size();
		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 0));
		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 1));
		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 2));

		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 0));
		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 2));
		meshComponent->Indices.push_back(static_cast<uint16_t>(vbase + 3));

		XMFLOAT3 positon;
		XMStoreFloat3(&positon, (normal - side1 - side2) * size);
		meshComponent->VertexPositions.push_back(positon);
		XMStoreFloat3(&positon, (normal - side1 + side2) * size);
		meshComponent->VertexPositions.push_back(positon);
		XMStoreFloat3(&positon, (normal + side1 + side2) * size);
		meshComponent->VertexPositions.push_back(positon);
		XMStoreFloat3(&positon, (normal + side1 - side2) * size);
		meshComponent->VertexPositions.push_back(positon);

		meshComponent->VertexTexCoords.push_back(textureCoordinates[0]);
		meshComponent->VertexTexCoords.push_back(textureCoordinates[1]);
		meshComponent->VertexTexCoords.push_back(textureCoordinates[2]);
		meshComponent->VertexTexCoords.push_back(textureCoordinates[3]);

		meshComponent->VertexColour.push_back(faceColour[0]);
		meshComponent->VertexColour.push_back(faceColour[1]);
		meshComponent->VertexColour.push_back(faceColour[2]);
		meshComponent->VertexColour.push_back(faceColour[3]);

		meshComponent->VertexNormals.push_back(faceNormals[i]);
		meshComponent->VertexNormals.push_back(faceNormals[i]);
		meshComponent->VertexNormals.push_back(faceNormals[i]);
		meshComponent->VertexNormals.push_back(faceNormals[i]);
	}

	meshComponent->ComputeTangentSpace();

	if (rhsCoords)
	{
		meshComponent->ReverseWinding();
	}

	meshComponent->TotalIndices = meshComponent->Indices.size();
	meshComponent->TotalVertices = meshComponent->VertexPositions.size();

	auto& geometry = meshComponent->Geometry.emplace_back();
	geometry.MaterialID = mtlID;
	geometry.NumIndices = meshComponent->TotalIndices;
	geometry.NumVertices= meshComponent->TotalVertices;
	return meshEntity;
}

ECS::Entity Legacy::Scene::CreateSphereMeshEntity(std::string const& name, ECS::Entity mtlID, float diameter, size_t tessellation, bool rhsCoords)
{
	if (tessellation < 3)
	{
		throw std::out_of_range("tessellation parameter out of range");
	}

	float radius = diameter / 2.0f;
	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	ECS::Entity meshEntity = this->EntityCreateMesh(name);
	auto* meshComponent = this->Meshes.GetComponent(meshEntity);
	
	// Create rings of vertices at progressively higher latitudes.
	for (size_t i = 0; i <= verticalSegments; i++)
	{
		float v = 1 - (float)i / verticalSegments;

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			float u = (float)j / horizontalSegments;

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);

			XMFLOAT3 positon;
			XMStoreFloat3(&positon, normal * radius);
			meshComponent->VertexPositions.push_back(positon);
			meshComponent->VertexTexCoords.push_back({ u, v });
			meshComponent->VertexNormals.push_back({ dx, dy, dz });
			meshComponent->VertexColour.push_back({ 0.1f, 0.1f, 0.1f });
			meshComponent->TotalVertices++;
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;
	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			meshComponent->Indices.push_back(static_cast<uint16_t>(i * stride + j));
			meshComponent->Indices.push_back(static_cast<uint16_t>(nextI * stride + j));
			meshComponent->Indices.push_back(static_cast<uint16_t>(i * stride + nextJ));

			meshComponent->Indices.push_back(static_cast<uint16_t>(i * stride + nextJ));
			meshComponent->Indices.push_back(static_cast<uint16_t>(nextI * stride + j));
			meshComponent->Indices.push_back(static_cast<uint16_t>(nextI * stride + nextJ));
		}
	}

	meshComponent->TotalIndices = meshComponent->Indices.size();

	meshComponent->ComputeTangentSpace();

	if (rhsCoords)
	{
		meshComponent->ReverseWinding();
	}

	auto& geometry = meshComponent->Geometry.emplace_back();
	geometry.MaterialID = mtlID;
	geometry.NumIndices = meshComponent->TotalIndices;
	geometry.NumVertices = meshComponent->TotalVertices;

	return meshEntity;
}

void Legacy::Scene::ComponentAttach(ECS::Entity entity, ECS::Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);

	// Detatch so we can ensure the child comes after the parent in the array.
	// This is to ensure that the parent's transforms are updated fully prior to reaching
	// any children.
	if (this->Hierarchy.Contains(entity))
	{
		this->ComponentDetach(entity);
	}

	HierarchyComponent& entityComponent = this->Hierarchy.Create(entity);
	entityComponent.ParentId = parent;

	TransformComponent* transformParent = this->Transforms.GetComponent(parent);
	if (transformParent == nullptr)
	{
		transformParent = &this->Transforms.Create(parent);
	}

	TransformComponent* transformChild = this->Transforms.GetComponent(entity);
	if (transformChild == nullptr)
	{
		transformChild = &this->Transforms.Create(entity);
		transformParent = this->Transforms.GetComponent(parent); // after transforms.Create(), transform_parent pointer could have become invalidated!
	}

	if (!childInLocalSpace)
	{
		XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transformParent->WorldMatrix));
		transformChild->MatrixTransform(B);
		transformChild->UpdateTransform();
	}

	transformChild->UpdateTransform(*transformParent);
}

void Legacy::Scene::ComponentDetach(ECS::Entity entity)
{
	const HierarchyComponent* parent = this->Hierarchy.GetComponent(entity);

	if (parent != nullptr)
	{
		TransformComponent* transform = this->Transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->ApplyTransform();
		}

		this->Hierarchy.Remove(entity);
	}
}

void Legacy::Scene::ComponentDetachChildren(ECS::Entity parent)
{
	for (size_t i = 0; i < this->Hierarchy.GetCount(); )
	{
		if (this->Hierarchy[i].ParentId == parent)
		{
			ECS::Entity entity = this->Hierarchy.GetEntity(i);
			this->ComponentDetach(entity);
		}
		else
		{
			++i;
		}
	}
}

void Legacy::Scene::RunMeshInstanceUpdateSystem()
{
	for (int i = 0; i < this->MeshInstances.GetCount(); i++)
	{
		auto& meshInstace = this->MeshInstances[i];
		meshInstace.RenderBucketMask = 0;

		ECS::Entity meshEntity = meshInstace.MeshId;

		auto& meshComponent = *this->Meshes.GetComponent(meshEntity);
		for (auto& geom : meshComponent.Geometry)
		{
			auto& materialEntity = geom.MaterialID;
			auto* materialComponent = this->Materials.GetComponent(materialEntity);

			if (materialComponent)
			{
				meshInstace.RenderBucketMask |= materialComponent->GetRenderTypes();
			}
		}
	}
}

void Legacy::Scene::RefreshGpuBuffers(RHI::IGraphicsDevice* graphicsDevice, RHI::CommandListHandle commandList)
{
	// let's construct the mesh buffers

	size_t numGeometry = 0ull;
	for (int i = 0; i < Meshes.GetCount(); i++)
	{
		MeshComponent& mesh = Meshes[i];
		mesh.CreateRenderData(graphicsDevice, commandList);

		// Determine the number of geometry data
		numGeometry += mesh.Geometry.size();
	}

	// Construct Geometry Data
	// TODO: Create a thread to collect geometry Count
	size_t geometryCounter = 0ull;
	if (this->m_geometryShaderData.size() != numGeometry)
	{
		this->m_geometryShaderData.resize(numGeometry);

		size_t globalGeomtryIndex = 0ull;
		for (int i = 0; i < Meshes.GetCount(); i++)
		{
			MeshComponent& mesh = Meshes[i];

			for (int j = 0; j < mesh.Geometry.size(); j++)
			{
				// Construct the Geometry data
				auto& geometryData = mesh.Geometry[j];
				geometryData.GlobalGeometryIndex = geometryCounter++;

				auto& geometryShaderData = this->m_geometryShaderData[geometryData.GlobalGeometryIndex];

				geometryShaderData.MaterialIndex = this->Materials.GetIndex(geometryData.MaterialID);
				geometryShaderData.NumIndices = geometryData.NumIndices;
				geometryShaderData.NumVertices = geometryData.NumVertices;
				geometryShaderData.IndexOffset = geometryData.IndexOffsetInMesh;
				geometryShaderData.VertexBufferIndex = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mesh.VertexGpuBuffer);
				geometryShaderData.PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
				geometryShaderData.TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
				geometryShaderData.NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
				geometryShaderData.TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
			}
		}

		// -- Create GPU Data ---
		BufferDesc desc = {};
		desc.DebugName = "Geometry Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
		desc.StrideInBytes = sizeof(Shader::Geometry);
		desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_geometryShaderData.size();

		this->GeometryGpuBuffer = graphicsDevice->CreateBuffer(desc);

		// Upload Data
		commandList->TransitionBarrier(this->GeometryGpuBuffer, ResourceStates::Common, ResourceStates::CopyDest);
		commandList->WriteBuffer(this->GeometryGpuBuffer, this->m_geometryShaderData);
		commandList->TransitionBarrier(this->GeometryGpuBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
	}

	// Construct Material Data
	if (this->Materials.GetCount() != this->m_materialShaderData.size())
	{
		// Create Material Data
		this->m_materialShaderData.resize(this->Materials.GetCount());

		for (int i = 0; i < this->Materials.GetCount(); i++)
		{
			auto& gpuMaterial = this->m_materialShaderData[i];
			this->Materials[i].PopulateShaderData(gpuMaterial);
		}

		// -- Create GPU Data ---
		BufferDesc desc = {};
		desc.DebugName = "Material Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
		desc.StrideInBytes = sizeof(Shader::MaterialData);
		desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_materialShaderData.size();

		this->MaterialBuffer = graphicsDevice->CreateBuffer(desc);

		// Upload Data
		commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::Common, ResourceStates::CopyDest);
		commandList->WriteBuffer(this->MaterialBuffer, this->m_materialShaderData);
		commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
	}
}

void Legacy::Scene::UpdateTansformsSystem()
{
	for (size_t i = 0; i < this->Transforms.GetCount(); i++)
	{
		auto& transformComp = this->Transforms[i];
		transformComp.UpdateTransform();
	}
}

void Legacy::Scene::UpdateHierarchySystem()
{
	for (size_t i = 0; i < this->Hierarchy.GetCount(); i++)
	{
		auto& hier = this->Hierarchy[i];
		ECS::Entity entity = this->Hierarchy.GetEntity(i);

		TransformComponent* childTransform = this->Transforms.GetComponent(entity);
		if (!childTransform)
		{
			continue;
		}

		XMMATRIX  worldMatrix = childTransform->GetLocalMatrix();

		ECS::Entity parentEntity = hier.ParentId;
		while (parentEntity != ECS::InvalidEntity)
		{
			// Get the parents transform
			TransformComponent* parentTransform = this->Transforms.GetComponent(parentEntity);
			if (parentTransform)
			{
				worldMatrix *= parentTransform->GetLocalMatrix();
			}

			HierarchyComponent* currentParentHeir = this->Hierarchy.GetComponent(parentEntity);

			parentEntity = currentParentHeir ? currentParentHeir->ParentId : ECS::InvalidEntity;
		}

		XMStoreFloat4x4(&childTransform->WorldMatrix, worldMatrix);
	}
}

void Legacy::Scene::UpdateLightsSystem()
{
	for (size_t i = 0; i < this->Lights.GetCount(); i++)
	{
		LightComponent& light = this->Lights[i];
		ECS::Entity entity = this->Lights.GetEntity(i);
		const TransformComponent& transform = *this->Transforms.GetComponent(entity);

		DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&transform.WorldMatrix);
		DirectX::XMVECTOR translationT;
		DirectX::XMVECTOR rotationV;
		DirectX::XMVECTOR scaleV;
		DirectX::XMMatrixDecompose(&scaleV, &rotationV, &translationT, world);

		DirectX::XMStoreFloat3(&light.Position, translationT);
		DirectX::XMStoreFloat4(&light.Rotation, rotationV);
		DirectX::XMStoreFloat3(&light.Scale, scaleV);
	}

}
