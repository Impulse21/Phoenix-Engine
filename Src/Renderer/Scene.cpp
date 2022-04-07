#include <PhxEngine/Renderer/Scene.h>

#include <DirectXMath.h>

#include <algorithm>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>
#include <Shaders/ShaderInteropStructures.h>

#include "..\..\Include\PhxEngine\Renderer\Scene.h"

using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace PhxEngine::ECS;
using namespace DirectX;

// Helper for flipping winding of geometric primitives for LH vs. RH coords
static void ReverseWinding(std::shared_ptr<MeshBuffers> meshBuffers)
{
	PHX_ASSERT((meshBuffers->IndexData.size() % 3) == 0);
	for (auto iter = meshBuffers->IndexData.begin(); iter != meshBuffers->IndexData.end(); iter += 3)
	{
		std::swap(*iter, *(iter + 2));
	}
}

// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#tangent-and-bitangent
static void ComputeTangentSpace(std::shared_ptr<MeshBuffers> meshBuffers)
{
	PHX_ASSERT(meshBuffers->IndexData.size() % 3 == 0);

	const size_t vertexCount = meshBuffers->PositionData.size();
	meshBuffers->TangentData.resize(vertexCount);

	std::vector<XMVECTOR> tangents(vertexCount);
	std::vector<XMVECTOR> bitangents(vertexCount);

	for (int i = 0; i < meshBuffers->IndexData.size(); i += 3)
	{
		auto& index0 = meshBuffers->IndexData[i + 0];
		auto& index1 = meshBuffers->IndexData[i + 1];
		auto& index2 = meshBuffers->IndexData[i + 2];

		// Vertices
		XMVECTOR pos0 = XMLoadFloat3(&meshBuffers->PositionData[index0]);
		XMVECTOR pos1 = XMLoadFloat3(&meshBuffers->PositionData[index1]);
		XMVECTOR pos2 = XMLoadFloat3(&meshBuffers->PositionData[index2]);

		// UVs
		XMVECTOR uvs0 = XMLoadFloat2(&meshBuffers->TexCoordData[index0]);
		XMVECTOR uvs1 = XMLoadFloat2(&meshBuffers->TexCoordData[index1]);
		XMVECTOR uvs2 = XMLoadFloat2(&meshBuffers->TexCoordData[index2]);

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

	for (int i = 0; i < vertexCount; i++)
	{
		const XMVECTOR normal = XMLoadFloat3(&meshBuffers->NormalData[i]);
		const XMVECTOR& tangent = tangents[i];
		const XMVECTOR& bitangent = bitangents[i];

		// Gram-Schmidt orthogonalize

		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		XMVectorSetW(tangent, sign);
		DirectX::XMStoreFloat4(&meshBuffers->TangentData[i], orthTangent);
	}
}

PhxEngine::Renderer::SceneGraph::SceneGraph()
{
	this->m_rootNode = std::make_shared<Node>();
}

PhxEngine::Renderer::Scene::Scene(
	std::shared_ptr<Core::IFileSystem> fileSystem,
	std::shared_ptr<TextureCache> textureCache,
	RHI::IGraphicsDevice* graphicsDevice)
	: m_fs(fileSystem)
	, m_textureCache(textureCache)
	, m_graphicsDevice(graphicsDevice)
	, m_resources(std::make_unique<Resources>())
{
}


CameraComponent& PhxEngine::Renderer::New::Scene::GetGlobalCamera()
{
	static CameraComponent mainCamera;
	return mainCamera;
}

Entity PhxEngine::Renderer::New::Scene::EntityCreateMeshInstance(std::string const& name)
{
	Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);

	MeshInstanceComponent& c = this->MeshInstances.Create(e);
	return e;
}

Entity PhxEngine::Renderer::New::Scene::EntityCreateCamera(
	std::string const& name,
	float width,
	float height,
	float nearPlane,
	float farPlane,
	float fov)
{
	Entity e = ECS::CreateEntity();

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

Entity New::Scene::EntityCreateMaterial(std::string const& name)
{
	Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	MaterialComponent& c = this->Materials.Create(e);
	
	return e;
}

Entity New::Scene::EntityCreateMesh(std::string const& name)
{
	Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	MeshComponent& c = this->Meshes.Create(e);

	return e;
}

Entity PhxEngine::Renderer::New::Scene::EntityCreateLight(std::string const& name)
{
	Entity e = ECS::CreateEntity();

	NameComponent& nameComponent = this->Names.Create(e);
	nameComponent.Name = name;

	this->Transforms.Create(e);
	this->Lights.Create(e);

	return e;
}

Entity New::Scene::CreateCubeMeshEntity(std::string const& name, Entity mtlID, float size, bool rhsCoords)
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

	Entity meshEntity = this->EntityCreateMesh(name);
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

	auto& geometry = meshComponent->Geometry.emplace_back();
	geometry.MaterialID = mtlID;
	geometry.NumIndices = meshComponent->TotalIndices;
	geometry.NumVertices= meshComponent->TotalVertices;
	return meshEntity;
}

void PhxEngine::Renderer::New::Scene::ComponentAttach(Entity entity, Entity parent, bool childInLocalSpace)
{
	PHX_ASSERT(entity != parent);

	// Detatch so we can ensure the child comes after the parent in the array.
	// This is to ensure that the parent's transforms are updated fully prior to reaching
	// any children.
	if (this->Hierarchy.Contains(entity))
	{
		this->ComponentDetach(entity);
	}

	HierarchyComponent& parentComponent = this->Hierarchy.Create(entity);
	parentComponent.ParentId = parent;

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

void PhxEngine::Renderer::New::Scene::ComponentDetach(ECS::Entity entity)
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

void PhxEngine::Renderer::New::Scene::ComponentDetachChildren(ECS::Entity parent)
{
	for (size_t i = 0; i < this->Hierarchy.GetCount(); )
	{
		if (this->Hierarchy[i].ParentId == parent)
		{
			Entity entity = this->Hierarchy.GetEntity(i);
			this->ComponentDetach(entity);
		}
		else
		{
			++i;
		}
	}
}

void PhxEngine::Renderer::New::Scene::RefreshGpuBuffers(RHI::IGraphicsDevice* graphicsDevice, RHI::CommandListHandle commandList)
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
				geometryShaderData.VertexBufferIndex = mesh.VertexGpuBuffer->GetDescriptorIndex();
				geometryShaderData.PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
				geometryShaderData.TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
				geometryShaderData.NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
				geometryShaderData.TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
			}
		}

		// -- Create GPU Data ---
		BufferDesc desc = {};
		desc.DebugName = "Geometry Data";
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::SrvView | RHI::BufferMiscFlags::Raw;
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
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::SrvView | RHI::BufferMiscFlags::Raw;
		desc.StrideInBytes = sizeof(Shader::MaterialData);
		desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_materialShaderData.size();

		this->MaterialBuffer = graphicsDevice->CreateBuffer(desc);

		// Upload Data
		commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::Common, ResourceStates::CopyDest);
		commandList->WriteBuffer(this->MaterialBuffer, this->m_materialShaderData);
		commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
	}
}

void PhxEngine::Renderer::New::Scene::PopulateShaderSceneData(Shader::SceneData& sceneData)
{
	sceneData.MaterialBufferIndex = this->MaterialBuffer->GetDescriptorIndex();
	sceneData.GeometryBufferIndex = this->GeometryGpuBuffer->GetDescriptorIndex();
	sceneData.IrradianceMapTexIndex = this->IrradanceMap.Get() ? this->IrradanceMap->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	sceneData.PreFilteredEnvMapTexIndex = this->PrefilteredMap.Get() ? this->PrefilteredMap->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
}

void PhxEngine::Renderer::New::Scene::UpdateTansformsSystem()
{
	for (size_t i = 0; i < this->Transforms.GetCount(); i++)
	{
		auto& transformComp = this->Transforms[i];
		transformComp.UpdateTransform();
	}
}

void PhxEngine::Renderer::New::Scene::UpdateHierarchySystem()
{
	for (size_t i = 0; i < this->Hierarchy.GetCount(); i++)
	{
		auto& hier = this->Hierarchy[i];
		Entity entity = this->Hierarchy.GetEntity(i);

		TransformComponent* childTransform = this->Transforms.GetComponent(entity);
		if (!childTransform)
		{
			continue;
		}

		XMMATRIX  worldMatrix = childTransform->GetLocalMatrix();

		Entity parentEntity = hier.ParentId;
		while (parentEntity != InvalidEntity)
		{
			// Get the parents transform
			TransformComponent* parentTransform = this->Transforms.GetComponent(parentEntity);
			if (parentTransform)
			{
				worldMatrix *= parentTransform->GetLocalMatrix();
			}

			HierarchyComponent* currentParentHeir = this->Hierarchy.GetComponent(parentEntity);

			parentEntity = currentParentHeir ? currentParentHeir->ParentId : InvalidEntity;
		}

		XMStoreFloat4x4(&childTransform->WorldMatrix, worldMatrix);
	}
}

void PhxEngine::Renderer::SceneGraph::Refresh()
{
	if (!this->m_isStructureDirty)
	{
		return;
	}

	// Update Mesh Geometry Indices
	int meshIndex = 0;
	int geometryIndex = 0;
	for (const auto& mesh : this->m_meshes)
	{
		for (const auto& geometry : mesh->Geometry)
		{
			geometry->GlobalGeometryIndex = geometryIndex;
			++geometryIndex;
		}

		mesh->GlobalMeshIndex = meshIndex;
		++meshIndex;
	}

	// Update Mesh Geometry Indices
	for (int i = 0; i < this->m_materials.size(); i++)
	{
		this->m_materials[i]->GlobalMaterialIndex = i;
	}

	this->m_isStructureDirty = false;
}

void SceneGraph::AttachNode(std::shared_ptr<Node> parent, std::shared_ptr<Node> child)
{
	parent->AttachChild(child);
	child->SetParentNode(parent.get());

	this->m_isStructureDirty = true;
}

void PhxEngine::Renderer::SceneGraphHelpers::InitializeSphereMesh(
	std::shared_ptr<PhxEngine::Renderer::Mesh> outMesh,
	std::shared_ptr<Material> material,
	float diameter,
	size_t tessellation,
	bool rhcoords)
{

	if (tessellation < 3)
	{
		LOG_CORE_ERROR("tessellation parameter out of range");
		throw std::out_of_range("tessellation parameter out of range");
	}

	float radius = diameter / 2.0f;
	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	std::shared_ptr<MeshGeometry> meshGeoemtry = std::make_shared<MeshGeometry>();
	meshGeoemtry->Material = material;

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
			outMesh->Buffers->PositionData.push_back(positon);
			outMesh->Buffers->TexCoordData.push_back({ u, v });
			outMesh->Buffers->NormalData.push_back({ dx, dy, dz });
			outMesh->Buffers->ColourData.push_back({ 0.1f, 0.1f, 0.1f });
			meshGeoemtry->NumVertices++;
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

			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(i * stride + j));
			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(nextI * stride + j));
			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(i * stride + nextJ));

			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(i * stride + nextJ));
			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(nextI * stride + j));
			outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(nextI * stride + nextJ));
		}
	}

	meshGeoemtry->NumIndices = outMesh->Buffers->IndexData.size();

	ComputeTangentSpace(outMesh->Buffers);

	if (rhcoords)
	{
		ReverseWinding(outMesh->Buffers);
	}


	outMesh->Geometry.push_back(meshGeoemtry);
	outMesh->TotalIndices = meshGeoemtry->NumIndices;
	outMesh->TotalVertices = meshGeoemtry->NumVertices;
}


void SceneGraphHelpers::InitializePlaneMesh(
	std::shared_ptr<Mesh> outMesh,
	std::shared_ptr<Material> material,
	float width,
	float height,
	bool rhcoords)
{
	outMesh->Buffers->PositionData =
	{
		XMFLOAT3(-0.5f * width, 0.0f,  0.5f * height),
		XMFLOAT3(0.5f * width, 0.0f,  0.5f * height),
		XMFLOAT3(0.5f * width, 0.0f, -0.5f * height),
		XMFLOAT3(-0.5f * width, 0.0f, -0.5f * height)
	};

	outMesh->Buffers->NormalData =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
	};

	outMesh->Buffers->TexCoordData =
	{
		 XMFLOAT2(0.0f, 0.0f),
		 XMFLOAT2(1.0f, 0.0f),
		 XMFLOAT2(1.0f, 1.0f),
		 XMFLOAT2(0.0f, 1.0f),
	};

	outMesh->Buffers->IndexData =
	{
		0, 3, 1, 1, 3, 2
	};


	ComputeTangentSpace(outMesh->Buffers);

	if (rhcoords)
	{
		ReverseWinding(outMesh->Buffers);
	}

	std::shared_ptr<MeshGeometry> meshGeoemtry = std::make_shared<MeshGeometry>();
	meshGeoemtry->Material = material;
	meshGeoemtry->NumIndices = outMesh->Buffers->IndexData.size();
	meshGeoemtry->NumVertices = outMesh->Buffers->PositionData.size();

	outMesh->Geometry.push_back(meshGeoemtry);
	outMesh->TotalIndices = meshGeoemtry->NumIndices;
	outMesh->TotalVertices = meshGeoemtry->NumVertices;

}

void SceneGraphHelpers::InitalizeCubeMesh(
	std::shared_ptr<Mesh> outMesh,
	std::shared_ptr<Material> material,
	float size,
	bool rhsCoord)
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

	size /= 2;

	for (int i = 0; i < FaceCount; i++)
	{
		XMVECTOR normal = XMLoadFloat3(&faceNormals[i]);

		// Get two vectors perpendicular both to the face normal and to each other.
		XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

		XMVECTOR side1 = XMVector3Cross(normal, basis);
		XMVECTOR side2 = XMVector3Cross(normal, side1);

		// Six IndexData (two triangles) per face.
		size_t vbase = outMesh->Buffers->PositionData.size();
		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 0));
		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 1));
		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 2));

		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 0));
		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 2));
		outMesh->Buffers->IndexData.push_back(static_cast<uint16_t>(vbase + 3));

		XMFLOAT3 positon;
		XMStoreFloat3(&positon, (normal - side1 - side2) * size);
		outMesh->Buffers->PositionData.push_back(positon);
		XMStoreFloat3(&positon, (normal - side1 + side2) * size);
		outMesh->Buffers->PositionData.push_back(positon);
		XMStoreFloat3(&positon, (normal + side1 + side2) * size);
		outMesh->Buffers->PositionData.push_back(positon);
		XMStoreFloat3(&positon, (normal + side1 - side2) * size);
		outMesh->Buffers->PositionData.push_back(positon);

		outMesh->Buffers->TexCoordData.push_back(textureCoordinates[0]);
		outMesh->Buffers->TexCoordData.push_back(textureCoordinates[1]);
		outMesh->Buffers->TexCoordData.push_back(textureCoordinates[2]);
		outMesh->Buffers->TexCoordData.push_back(textureCoordinates[3]);


		outMesh->Buffers->ColourData.push_back(faceColour[0]);
		outMesh->Buffers->ColourData.push_back(faceColour[1]);
		outMesh->Buffers->ColourData.push_back(faceColour[2]);
		outMesh->Buffers->ColourData.push_back(faceColour[3]);

		outMesh->Buffers->NormalData.push_back(faceNormals[i]);
		outMesh->Buffers->NormalData.push_back(faceNormals[i]);
		outMesh->Buffers->NormalData.push_back(faceNormals[i]);
		outMesh->Buffers->NormalData.push_back(faceNormals[i]);
	}

	ComputeTangentSpace(outMesh->Buffers);

	if (rhsCoord)
	{
		ReverseWinding(outMesh->Buffers);
	}


	std::shared_ptr<MeshGeometry> meshGeoemtry = std::make_shared<MeshGeometry>();
	meshGeoemtry->Material = material;
	meshGeoemtry->NumIndices = outMesh->Buffers->IndexData.size();
	meshGeoemtry->NumVertices = outMesh->Buffers->PositionData.size();

	outMesh->Geometry.push_back(meshGeoemtry);
	outMesh->TotalIndices = meshGeoemtry->NumIndices;
	outMesh->TotalVertices = meshGeoemtry->NumVertices;
}

void Scene::RefreshGpuBuffers(ICommandList* commandList)
{
	this->m_sceneGraph->Refresh();

	this->CreateMeshBuffers(commandList);

	if (!this->m_sceneGraph->GetMeshes().empty())
	{
		this->m_resources->GeometryData.resize(this->m_sceneGraph->GetMeshes().size());
		// this->m_geometryBuffer = this->CreateGeometryBuffer();
	}

	if (!this->m_sceneGraph->GetMaterials().empty())
	{
		this->m_resources->MaterialData.resize(this->m_sceneGraph->GetMaterials().size());
		// this->m_materialBuffer = this->CreateMaterialBuffer();
	}

	if (!this->m_sceneGraph->GetMeshInstanceNodes().empty())
	{
		this->m_resources->MeshInstanceData.resize(this->m_sceneGraph->GetMeshInstanceNodes().size());
		// this->m_instanceBuffer = this->CreateInstanceBuffer();
	}

	for (const auto& mesh : this->m_sceneGraph->GetMeshes())
	{
		this->UpdateGeometryData(mesh);
	}

	if (this->m_geometryBuffer)
	{
		this->WriteGeometryBuffer(commandList);
	}

	for (const auto& instance : this->m_sceneGraph->GetMeshInstanceNodes())
	{
		this->UpdateInstanceData(instance);
	}

	if (this->m_instanceBuffer)
	{

		this->WriteInstanceBuffer(commandList);
	}

	for (const auto& material : this->m_sceneGraph->GetMaterials())
	{
		this->UpdateMaterialData(material);
	}

	if (this->m_materialBuffer)
	{
		this->WriteMaterialBuffer(commandList);
	}
}

void Scene::CreateMeshBuffers(ICommandList* commandList)
{
	for (const auto& mesh : this->m_sceneGraph->GetMeshes())
	{
		if (!mesh->Buffers)
		{
			continue;
		}

		auto buffers = mesh->Buffers;

		if (!buffers->IndexData.empty() && !buffers->IndexGpuBuffer)
		{
			RHI::BufferDesc indexBufferDesc = {};
			indexBufferDesc.SizeInBytes = sizeof(uint32_t) * buffers->IndexData.size();
			indexBufferDesc.StrideInBytes = sizeof(uint32_t);
			indexBufferDesc.DebugName = "Index Buffer";
			indexBufferDesc.CreateSRVViews = true;
			indexBufferDesc.CreateBindless = true;
			buffers->IndexGpuBuffer = this->m_graphicsDevice->CreateIndexBuffer(indexBufferDesc);

			commandList->TransitionBarrier(buffers->IndexGpuBuffer, ResourceStates::Common, ResourceStates::CopyDest);
			commandList->WriteBuffer<uint32_t>(buffers->IndexGpuBuffer, buffers->IndexData);
			commandList->TransitionBarrier(buffers->IndexGpuBuffer, ResourceStates::CopyDest, ResourceStates::IndexGpuBuffer | ResourceStates::ShaderResource);

			// TODO, clear CPU data sonce the command list takes ownership of data.

		}

		if (!buffers->VertexBuffer)
		{
			RHI::BufferDesc desc = {};
			desc.SizeInBytes = 0;
			desc.StrideInBytes = sizeof(float);
			desc.DebugName = "Vertex Buffer";
			desc.CreateBindless = true;
			desc.CreateSRVViews = true;

			if (!buffers->PositionData.empty())
			{
				auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Position);
				bufferRange.ByteOffset = desc.SizeInBytes;
				bufferRange.SizeInBytes = buffers->PositionData.size() * sizeof(buffers->PositionData[0]);
				desc.SizeInBytes += bufferRange.SizeInBytes;
			}

			if (!buffers->TexCoordData.empty())
			{
				auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::TexCoord);
				bufferRange.ByteOffset = desc.SizeInBytes;
				bufferRange.SizeInBytes = buffers->TexCoordData.size() * sizeof(buffers->TexCoordData[0]);
				desc.SizeInBytes += bufferRange.SizeInBytes;
			}

			if (!buffers->NormalData.empty())
			{
				auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Normal);
				bufferRange.ByteOffset = desc.SizeInBytes;
				bufferRange.SizeInBytes = buffers->NormalData.size() * sizeof(buffers->NormalData[0]);
				desc.SizeInBytes += bufferRange.SizeInBytes;
			}

			if (!buffers->TangentData.empty())
			{
				auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Tangent);
				bufferRange.ByteOffset = desc.SizeInBytes;
				bufferRange.SizeInBytes = buffers->TangentData.size() * sizeof(buffers->TangentData[0]);
				desc.SizeInBytes += bufferRange.SizeInBytes;
			}

			buffers->VertexBuffer = this->m_graphicsDevice->CreateVertexBuffer(desc);

			// Write Data
			commandList->TransitionBarrier(buffers->VertexBuffer, ResourceStates::Common, ResourceStates::CopyDest);

			if (!buffers->PositionData.empty())
			{
				const auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Position);
				commandList->WriteBuffer(buffers->VertexBuffer, buffers->PositionData, bufferRange.ByteOffset);
			}

			if (!buffers->TexCoordData.empty())
			{
				const auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::TexCoord);
				commandList->WriteBuffer(buffers->VertexBuffer, buffers->TexCoordData, bufferRange.ByteOffset);
			}

			if (!buffers->NormalData.empty())
			{
				const auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Normal);
				commandList->WriteBuffer(buffers->VertexBuffer, buffers->NormalData, bufferRange.ByteOffset);
			}

			if (!buffers->TangentData.empty())
			{
				const auto& bufferRange = buffers->GetVertexAttribute(VertexAttribute::Tangent);
				commandList->WriteBuffer(buffers->VertexBuffer, buffers->TangentData, bufferRange.ByteOffset);
			}

			commandList->TransitionBarrier(buffers->VertexBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource | ResourceStates::VertexBuffer);
		}
	}
}

BufferHandle PhxEngine::Renderer::Scene::CreateGeometryBuffer()
{
	BufferDesc desc = {};
	desc.CreateBindless = true;
	desc.CreateSRVViews = true;
	desc.StrideInBytes = sizeof(GeometryData);
	desc.SizeInBytes = this->m_resources->GeometryData.size() * sizeof(GeometryData);
	desc.DebugName = "Bindless Geometry Data";

	return this->m_graphicsDevice->CreateBuffer(desc);
}

BufferHandle PhxEngine::Renderer::Scene::CreateMaterialBuffer()
{
	BufferDesc desc = {};
	desc.CreateBindless = true;
	desc.CreateSRVViews = true;
	desc.StrideInBytes = sizeof(MaterialData);
	desc.SizeInBytes = this->m_resources->MaterialData.size() * sizeof(MaterialData);
	desc.DebugName = "Bindless Material Data";

	return this->m_graphicsDevice->CreateBuffer(desc);
}

BufferHandle PhxEngine::Renderer::Scene::CreateInstanceBuffer()
{
	BufferDesc desc = {};
	desc.CreateBindless = true;
	desc.CreateSRVViews = true;
	desc.StrideInBytes = sizeof(MeshInstanceData);
	desc.SizeInBytes = this->m_resources->MeshInstanceData.size() * sizeof(MeshInstanceData);
	desc.DebugName = "Bindless Mesh Instance Data Data";

	return this->m_graphicsDevice->CreateBuffer(desc);
}

void PhxEngine::Renderer::Scene::UpdateGeometryData(std::shared_ptr<Mesh> const& mesh)
{
	for (const auto& geometry : mesh->Geometry)
	{
		uint32_t indexOffset = mesh->IndexOffset + geometry->IndexOffsetInMesh;
		uint32_t vertexOffset = mesh->VertexOffset + geometry->VertexOffsetInMesh;

		auto& gData = this->m_resources->GeometryData[geometry->GlobalGeometryIndex];
		gData.NumVertices = geometry->NumVertices;
		gData.NumIndices = geometry->NumIndices;
		gData.IndexBufferIndex = mesh->Buffers->IndexGpuBuffer->GetDescriptorIndex();
		gData.IndexOffset = indexOffset;
		gData.VertexBufferIndex = mesh->Buffers->VertexBuffer->GetDescriptorIndex();
		gData.MaterialIndex = geometry->Material->GlobalMaterialIndex;

		gData.PositionOffset = mesh->Buffers->HasVertexAttribuite(VertexAttribute::Position)
			? static_cast<uint32_t>(vertexOffset * sizeof(DirectX::XMFLOAT3) + mesh->Buffers->GetVertexAttribute(VertexAttribute::Position).ByteOffset)
			: ~0U;

		gData.TexCoordOffset = mesh->Buffers->HasVertexAttribuite(VertexAttribute::TexCoord)
			? static_cast<uint32_t>(vertexOffset * sizeof(DirectX::XMFLOAT2) + mesh->Buffers->GetVertexAttribute(VertexAttribute::TexCoord).ByteOffset)
			: ~0U;

		gData.NormalOffset = mesh->Buffers->HasVertexAttribuite(VertexAttribute::Normal)
			? static_cast<uint32_t>(vertexOffset * sizeof(DirectX::XMFLOAT3) + mesh->Buffers->GetVertexAttribute(VertexAttribute::Normal).ByteOffset)
			: ~0U;

		gData.TangentOffset = mesh->Buffers->HasVertexAttribuite(VertexAttribute::Tangent)
			? static_cast<uint32_t>(vertexOffset * sizeof(DirectX::XMFLOAT4) + mesh->Buffers->GetVertexAttribute(VertexAttribute::Tangent).ByteOffset)
			: ~0U;

		/*
		gData.ColourOffset = mesh->Buffers->HasVertexAttribuite(VertexAttribute::Colour)
			? static_cast<uint32_t>(vertexOffset * sizeof(DirectX::XMFLOAT3) + mesh->Buffers->GetVertexAttribute(VertexAttribute::Colour).ByteOffset)
			: ~0U;
			*/
	}
}

void PhxEngine::Renderer::Scene::WriteGeometryBuffer(RHI::ICommandList* commandList)
{
	commandList->TransitionBarrier(this->m_geometryBuffer, ResourceStates::Common, ResourceStates::CopyDest);
	commandList->WriteBuffer(this->m_geometryBuffer, this->m_resources->GeometryData);
	commandList->TransitionBarrier(this->m_geometryBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
}

void PhxEngine::Renderer::Scene::UpdateMaterialData(std::shared_ptr<Material> const& material)
{
	auto& mData = this->m_resources->MaterialData[material->GlobalMaterialIndex];
	mData.AlbedoColour = material->Albedo;
	mData.AlbedoTexture = material->AlbedoTexture ? material->AlbedoTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mData.NormalTexture = material->NormalMapTexture ? material->NormalMapTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;

	if (material->UseMaterialTexture)
	{
		mData.MaterialTexture = material->MaterialTexture ? material->MaterialTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	}

	mData.RoughnessTexture = material->RoughnessTexture ? material->RoughnessTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mData.MetalnessTexture = material->MetalnessTexture ? material->MetalnessTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;
	mData.AOTexture = material->AoTexture ? material->AoTexture->GetDescriptorIndex() : INVALID_DESCRIPTOR_INDEX;

	mData.Metalness = material->Metalness;
	mData.Rougness = material->Roughness;
	mData.AO = material->Ao;
}

void PhxEngine::Renderer::Scene::WriteMaterialBuffer(RHI::ICommandList* commandList)
{
	commandList->TransitionBarrier(this->m_materialBuffer, ResourceStates::Common, ResourceStates::CopyDest);
	commandList->WriteBuffer(this->m_materialBuffer, this->m_resources->MaterialData);
	commandList->TransitionBarrier(this->m_materialBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
}

void PhxEngine::Renderer::Scene::UpdateInstanceData(std::shared_ptr<MeshInstanceNode> const& meshInstanceNode)
{
	auto& idata = this->m_resources->MeshInstanceData[meshInstanceNode->GetInstanceIndex()];
	XMStoreFloat4x4(&idata.Transform, XMMatrixTranspose(meshInstanceNode->GetWorldMatrix()));

	const auto& mesh = meshInstanceNode->GetMeshData();
	idata.FirstGeometryIndex = mesh->Geometry[0]->GlobalGeometryIndex;
	idata.NumGeometries = uint32_t(mesh->Geometry.size());
}

void PhxEngine::Renderer::Scene::WriteInstanceBuffer(RHI::ICommandList* commandList)
{
	commandList->TransitionBarrier(this->m_instanceBuffer, ResourceStates::Common, ResourceStates::CopyDest);
	commandList->WriteBuffer(this->m_instanceBuffer, this->m_resources->MeshInstanceData);
	commandList->TransitionBarrier(this->m_instanceBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
}

void PhxEngine::Renderer::New::PrintScene(Scene const& scene, std::stringstream& stream)
{
	auto WriteEntity = [&](Entity e)
	{
		stream << "\t\t\t" << "Entity" << ": " << e << std::endl;
	};
	auto WriteNameComponent = [&](const NameComponent* nameComponent)
	{
		stream << "\t\tName: ";
		if (nameComponent)
		{
			stream << nameComponent->Name << std::endl;
		}
		else
		{
			stream << "Unknown" << std::endl;
		}
	};

	auto WriteTexture = [&](std::string const& textureName, TextureHandle handle)
	{
		stream << "\t\t\t" << textureName << " Texture Descriptor Index: ";
		if (handle)
		{
			stream << handle->GetDescriptorIndex();
		}
		else
		{
			stream << INVALID_DESCRIPTOR_INDEX;
		}

		stream << std::endl;
	}; // end of lambda expression

	auto WriteFloat = [&](std::string const& title, float v)
	{
		stream << "\t\t\t" << title << ": " << v << std::endl;
	}; // end of lambda expression

	auto WriteUint32 = [&](std::string const& title, uint32_t v)
	{
		stream << "\t\t\t" << title << ": " << v << std::endl;
	}; // end of lambda expression

	auto WriteSize = [&](std::string const& title, size_t v)
	{
		stream << "\t\t\t" << title << ": " << v << std::endl;
	}; // end of lambda expression

	auto WriteFloat4 = [&](std::string const& title, XMFLOAT4 const& float4)
	{
		stream << "\t\t\t" << title << ": ";
		stream << "[ " << float4.x << ", " << float4.y << ", " << float4.z << ", " << float4.w << "]" << std::endl;
	}; // end of lambda expression

	auto WriteBool = [&](std::string const& title, bool boolean)
	{
		stream << "\t\t\t" << title << ": " << (boolean ? "true" : "false") << std::endl;
	}; // end of lambda expression


	std::string retVal;
	stream << "Scene Data" << std::endl;

	stream << "\tMaterials [" << scene.Materials.GetCount() << "]: " << std::endl;
	for (int i = 0; i < scene.Materials.GetCount(); i++)
	{
		
		const MaterialComponent mtlComponent = scene.Materials[i];
		ECS::Entity e = scene.Materials.GetEntity(i);

		const NameComponent* mtlName = scene.Names.GetComponent(e);
		WriteEntity(e);
		WriteNameComponent(mtlName);

		// Write base colour
		WriteFloat4("Albedo", mtlComponent.Albedo);
		WriteTexture("Abledo", mtlComponent.AlbedoTexture);

		WriteFloat("Metalness", mtlComponent.Metalness);
		WriteFloat("Roughness", mtlComponent.Roughness);
		WriteTexture("MetalRoughness", mtlComponent.MetalRoughnessTexture);

		WriteTexture("Normal Map", mtlComponent.NormalMapTexture);
		WriteBool("Is Double Sided", mtlComponent.IsDoubleSided);
		stream << std::endl;
	}

	stream << "\tMesh [" << scene.Meshes.GetCount() << "]:" << std::endl;

	for (int i = 0; i < scene.Meshes.GetCount(); i++)
	{
		const MeshComponent component = scene.Meshes[i];
		ECS::Entity e = scene.Meshes.GetEntity(i);

		const NameComponent* nameComponent = scene.Names.GetComponent(e);
		WriteEntity(e);
		WriteNameComponent(nameComponent);

		WriteUint32("Index Offset", component.IndexOffset);
		WriteUint32("Vertex Offset", component.VertexOffset);
		WriteUint32("Total Indices", component.TotalIndices);
		WriteUint32("Total Vertices", component.TotalVertices);
		// WriteSize("Global Mesh Index", component.GlobalMeshIndex);

		stream << "\t\tMesh Geometry [" << scene.Meshes.GetCount() << "]:" << std::endl;
		for (auto& geom : component.Geometry)
		{
			WriteUint32("\tMaterial Index", geom.MaterialID);
			WriteUint32("\tIndex Offset", geom.IndexOffsetInMesh);
			WriteUint32("\tVertex Offset", geom.VertexOffsetInMesh);
			WriteUint32("\tTotal Indices", geom.NumVertices);
			WriteUint32("\tTotal Vertices", geom.NumIndices);
			WriteSize("\tGlobal Geometry Index", geom.GlobalGeometryIndex);
			stream << std::endl;
		}

		stream << std::endl;
	}

	stream << "\tMesh Instances [" << scene.MeshInstances.GetCount() << "]:" << std::endl;
	for (int i = 0; i < scene.MeshInstances.GetCount(); i++)
	{
		const MeshInstanceComponent miComp = scene.MeshInstances[i];
		Entity e = scene.MeshInstances.GetEntity(i);

		WriteEntity(e);

		const NameComponent* nameComponent = scene.Names.GetComponent(e);
		WriteNameComponent(nameComponent);

		WriteSize("Transform Compoment", scene.Transforms.GetIndex(e));
		WriteSize("Hierarchy Component", scene.Hierarchy.GetIndex(e));
		WriteSize("MeshID", miComp.MeshId);
	}
}
