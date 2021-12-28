#include <PhxEngine/Renderer/Scene.h>

#include <DirectXMath.h>

#include <algorithm>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>

using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
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

		if (!buffers->IndexData.empty() && !buffers->IndexBuffer)
		{
			RHI::BufferDesc indexBufferDesc = {};
			indexBufferDesc.SizeInBytes = sizeof(uint32_t) * buffers->IndexData.size();
			indexBufferDesc.StrideInBytes = sizeof(uint32_t);
			indexBufferDesc.DebugName = "Index Buffer";
			indexBufferDesc.CreateSRVViews = true;
			indexBufferDesc.CreateBindless = true;
			buffers->IndexBuffer = this->m_graphicsDevice->CreateIndexBuffer(indexBufferDesc);

			commandList->TransitionBarrier(buffers->IndexBuffer, ResourceStates::Common, ResourceStates::CopyDest);
			commandList->WriteBuffer<uint32_t>(buffers->IndexBuffer, buffers->IndexData);
			commandList->TransitionBarrier(buffers->IndexBuffer, ResourceStates::CopyDest, ResourceStates::IndexBuffer | ResourceStates::ShaderResource);

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
		gData.IndexBufferIndex = mesh->Buffers->IndexBuffer->GetDescriptorIndex();
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
