#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Memory.h>

#include <PhxEngine/Core/Helpers.h>
#include <DirectXMesh.h>
#include <PhxEngine/Renderer/Renderer.h>


using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

constexpr static uint64_t kVertexBufferAlignment = 16ull;

namespace
{
	// An integer version of ceil(value / divisor)
	template <typename T, typename U>
	T DivRoundUp(T num, U denom)
	{
		return (num + denom - 1) / denom;
	}
}

void MeshComponent::BuildRenderData(
	Core::IAllocator* allocator,
	RHI::IGraphicsDevice* gfxDevice)
{
	RHI::ICommandList* commandList = gfxDevice->BeginCommandRecording();

	// Construct the Mesh buffer
	if (this->Indices && !this->IndexBuffer.IsValid())
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = sizeof(uint32_t) * this->TotalIndices;
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexBuffer = gfxDevice->CreateIndexBuffer(indexBufferDesc);

		commandList->TransitionBarrier(this->IndexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(this->IndexBuffer, Core::Span(this->Indices, this->TotalIndices));

		commandList->TransitionBarrier(
			this->IndexBuffer,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource);
	}

	Renderer::ResourceUpload vertexUpload = {};
	if (this->Positions && !this->VertexBuffer.IsValid())
	{
		size_t vertexBufferSize = Helpers::AlignTo(this->TotalVertices * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);

		if (this->Normals && (this->Flags & MeshComponent::Flags::kContainsNormals) == MeshComponent::Flags::kContainsNormals)
		{
			vertexBufferSize += Helpers::AlignTo(this->TotalVertices * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);
		}

		if (this->TexCoords && (this->Flags & MeshComponent::Flags::kContainsTexCoords) == MeshComponent::Flags::kContainsTexCoords)
		{
			vertexBufferSize += Helpers::AlignTo(this->TotalVertices * sizeof(DirectX::XMFLOAT2), kVertexBufferAlignment);
		}

		if (this->Tangents && (this->Flags & MeshComponent::Flags::kContainsTangents) == MeshComponent::Flags::kContainsTangents)
		{
			vertexBufferSize += Helpers::AlignTo(this->TotalVertices * sizeof(DirectX::XMFLOAT4), kVertexBufferAlignment);
		}

		RHI::BufferDesc vertexDesc = {};
		vertexDesc.StrideInBytes = sizeof(float);
		vertexDesc.DebugName = "Vertex Buffer";
		vertexDesc.EnableBindless();
		vertexDesc.IsRawBuffer();
		vertexDesc.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource;
		vertexDesc.SizeInBytes = vertexBufferSize;

		// Is this Needed for Raw Buffer Type
		this->VertexBuffer = gfxDevice->CreateVertexBuffer(vertexDesc);
		vertexUpload = Renderer::CreateResourceUpload(vertexDesc.SizeInBytes);

		std::vector<uint8_t> gpuBufferData(vertexDesc.SizeInBytes);
		std::memset(gpuBufferData.data(), 0, vertexDesc.SizeInBytes);
		uint64_t bufferOffset = 0ull;

		uint64_t startOffset = vertexUpload.Offset;

		auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* Data, uint64_t sizeInBytes)
		{
			auto& bufferRange = this->GetVertexAttribute(attr);
			bufferRange.ByteOffset = bufferOffset;
			bufferRange.SizeInBytes = sizeInBytes;
			bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, kVertexBufferAlignment);
			vertexUpload.SetData(Data, bufferRange.SizeInBytes, kVertexBufferAlignment);
		};

		if (this->Positions)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Position,
				this->Positions,
				this->TotalVertices * sizeof(DirectX::XMFLOAT3));
		}

		if (this->TexCoords)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::TexCoord,
				this->TexCoords,
				this->TotalVertices * sizeof(DirectX::XMFLOAT2));
		}

		if (this->Normals)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Normal,
				this->Normals,
				this->TotalVertices * sizeof(DirectX::XMFLOAT3));
		}

		if (this->Tangents)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Tangent,
				this->Tangents,
				this->TotalVertices * sizeof(DirectX::XMFLOAT4));
		}

		if (false)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Colour,
				this->Colour,
				this->TotalVertices * sizeof(DirectX::XMFLOAT3));
		}

		// Write Data
		commandList->TransitionBarrier(this->VertexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->CopyBuffer(
			this->VertexBuffer,
			0,
			vertexUpload.UploadBuffer,
			startOffset,
			vertexDesc.SizeInBytes);
		commandList->TransitionBarrier(this->VertexBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
	}


	if (!this->Meshlets.empty() && !this->MeshletBuffer.IsValid())
	{
		this->MeshletBuffer = gfxDevice->CreateBuffer({
				.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.StrideInBytes = sizeof(DirectX::Meshlet),
				.SizeInBytes = this->Meshlets.size() * sizeof(DirectX::Meshlet),
				.DebugName = "Meshlet Buffer" });

		this->UniqueVertexIBBuffer = gfxDevice->CreateBuffer({
				.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.SizeInBytes = this->UniqueVertexIB.size() * sizeof(uint8_t),
				.DebugName = "Meshlet Unique Vertex IB" });

		this->MeshletPrimitivesBuffer = gfxDevice->CreateBuffer({
				.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.SizeInBytes = this->MeshletTriangles.size() * sizeof(DirectX::MeshletTriangle),
				.DebugName = "Meshlet Primitives" });

		this->MeshletCullDataBuffer = gfxDevice->CreateBuffer({
				.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.StrideInBytes = sizeof(DirectX::CullData),
				.SizeInBytes = this->Meshlets.size() * sizeof(DirectX::CullData),
				.DebugName = "Meshlet Cull Data" });

		RHI::GpuBarrier preCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(this->MeshletBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->UniqueVertexIBBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->MeshletPrimitivesBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->MeshletCullDataBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

		commandList->WriteBuffer(this->MeshletBuffer, this->Meshlets);
		commandList->WriteBuffer(this->UniqueVertexIBBuffer, this->UniqueVertexIB);
		commandList->WriteBuffer(this->MeshletPrimitivesBuffer, this->MeshletTriangles);
		commandList->WriteBuffer(this->MeshletCullDataBuffer, this->MeshletCullData);

		RHI::GpuBarrier postCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(this->MeshletBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->UniqueVertexIBBuffer, RHI::ResourceStates::CopyDest,  RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->MeshletPrimitivesBuffer, RHI::ResourceStates::CopyDest,  RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->MeshletCullDataBuffer, RHI::ResourceStates::CopyDest,  RHI::ResourceStates::ShaderResource),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
	}

	commandList->Close();
	gfxDevice->ExecuteCommandLists({ commandList });

	// Create RT BLAS object
	if (gfxDevice->CheckCapability(RHI::DeviceCapability::RayTracing))
	{
		this->BlasState = BLASState::Rebuild;

		// Create Blast
		RHI::RTAccelerationStructureDesc rtDesc = {};
		rtDesc.Type = RHI::RTAccelerationStructureDesc::Type::BottomLevel;
		rtDesc.Flags == RHI::RTAccelerationStructureDesc::kPreferFastTrace;

		auto& geometry = rtDesc.ButtomLevel.Geometries.emplace_back();
		geometry.Type = RHI::RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles;
		geometry.Triangles.VertexBuffer = this->VertexBuffer;
		geometry.Triangles.VertexStride = sizeof(DirectX::XMFLOAT3);
		geometry.Triangles.VertexByteOffset = 0;
		geometry.Triangles.VertexCount = TotalVertices;
		geometry.Triangles.VertexFormat = RHI::RHIFormat::RGB32_FLOAT;

		geometry.Triangles.IndexBuffer = this->IndexBuffer;
		geometry.Triangles.IndexCount = TotalIndices;
		geometry.Triangles.IndexOffset = 0;
	
		this->Blas = gfxDevice->CreateRTAccelerationStructure(rtDesc);
	}
}

void PhxEngine::Scene::MeshComponent::ReverseWinding()
{
	assert((this->TotalIndices % 3) == 0);
	for (size_t i = 0; i < this->TotalIndices; i += 3)
	{
		std::swap(this->Indices[i + 1], this->Indices[i + 2]);
	}
}

void PhxEngine::Scene::MeshComponent::InitializeCpuBuffers(IAllocator* allocator)
{
	this->Positions = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * this->TotalVertices, 0));
	this->TexCoords = reinterpret_cast<DirectX::XMFLOAT2*>(allocator->Allocate(sizeof(float) * 2 * this->TotalVertices, 0));
	this->Normals = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * this->TotalVertices, 0));
	this->Tangents = reinterpret_cast<DirectX::XMFLOAT4*>(allocator->Allocate(sizeof(float) * 4 * this->TotalVertices, 0));
	this->Colour = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * this->TotalVertices, 0));
	this->Indices = reinterpret_cast<uint32_t*>(allocator->Allocate(sizeof(uint32_t) * this->TotalIndices, 0));
}

void PhxEngine::Scene::MeshComponent::ComputeTangentSpace()
{
	std::vector<DirectX::XMVECTOR> computedTangents(this->TotalVertices);
	std::vector<DirectX::XMVECTOR> computedBitangents(this->TotalVertices);

	for (int i = 0; i < this->TotalIndices; i += 3)
	{
		auto& index0 = this->Indices[i + 0];
		auto& index1 = this->Indices[i + 1];
		auto& index2 = this->Indices[i + 2];

		// Vertices
		DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&this->Positions[index0]);
		DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&this->Positions[index1]);
		DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&this->Positions[index2]);

		// UVs
		DirectX::XMVECTOR uvs0 = DirectX::XMLoadFloat2(&this->TexCoords[index0]);
		DirectX::XMVECTOR uvs1 = DirectX::XMLoadFloat2(&this->TexCoords[index1]);
		DirectX::XMVECTOR uvs2 = DirectX::XMLoadFloat2(&this->TexCoords[index2]);

		DirectX::XMVECTOR deltaPos1 = DirectX::XMVectorSubtract(pos1, pos0);
		DirectX::XMVECTOR deltaPos2 = DirectX::XMVectorSubtract(pos2, pos0);

		DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uvs1, uvs0);
		DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uvs2, uvs0);

		// TODO: Take advantage of SIMD better here
		float r = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));

		DirectX::XMVECTOR tangent = (deltaPos1 * DirectX::XMVectorGetY(deltaUV2) - deltaPos2 * DirectX::XMVectorGetY(deltaUV1)) * r;
		DirectX::XMVECTOR bitangent = (deltaPos2 * DirectX::XMVectorGetX(deltaUV1) - deltaPos1 * DirectX::XMVectorGetX(deltaUV2)) * r;

		computedTangents[index0] += tangent;
		computedTangents[index1] += tangent;
		computedTangents[index2] += tangent;

		computedBitangents[index0] += bitangent;
		computedBitangents[index1] += bitangent;
		computedBitangents[index2] += bitangent;
	}

	for (int i = 0; i < this->TotalVertices; i++)
	{
		const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&this->Normals[i]);
		const DirectX::XMVECTOR& tangent = computedTangents[i];
		const DirectX::XMVECTOR& bitangent = computedBitangents[i];

		// Gram-Schmidt orthogonalize
		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		orthTangent = DirectX::XMVectorSetW(orthTangent, sign);
		DirectX::XMStoreFloat4(&this->Tangents[i], orthTangent);
	}
}

void PhxEngine::Scene::MeshComponent::FlipZ()
{
	// Flip Z
	for (int i = 0; i < this->TotalVertices; i++)
	{
		this->Positions[i].z *= -1.0f;
	}
	for (int i = 0; i < this->TotalVertices; i++)
	{
		this->Normals[i].z *= -1.0f;
	}
	for (int i = 0; i < this->TotalVertices; i++)
	{
		this->Tangents[i].z *= -1.0f;
	}
}

void PhxEngine::Scene::MeshComponent::ComputeMeshletData(IAllocator* allocator)
{
	// Recomended for NVIDA, which is the GPU I am testing.
	auto hr = DirectX::ComputeMeshlets(
		this->Indices, this->TotalIndices / 3,
		this->Positions, this->TotalVertices,
		nullptr,
		this->Meshlets,
		this->UniqueVertexIB,
		this->MeshletTriangles,
		MAX_VERTS,
		MAX_PRIMS);
	assert(SUCCEEDED(hr));

	this->PackedVertexData = reinterpret_cast<Shader::New::MeshletPackedVertexData*>(allocator->Allocate(sizeof(Shader::New::MeshletPackedVertexData) * this->TotalVertices, 0));
	for (int i = 0; i < this->TotalVertices; i++)
	{
		Shader::New::MeshletPackedVertexData& vertex = this->PackedVertexData[i];
		vertex.SetNormal({ this->Normals[i].x, this->Normals[i].y, this->Normals[i].z, 1.0f });
		vertex.SetTangent(this->Tangents[i]);
		vertex.SetTexCoord(this->TexCoords[i]);
	}

	this->MeshletCullData = reinterpret_cast<DirectX::CullData*>(allocator->Allocate(sizeof(DirectX::CullData) * this->Meshlets.size(), 0));

	// Get Meshlet Culling data
	DirectX::ComputeCullData(
		this->Positions, this->TotalVertices,
		this->Meshlets.data(), this->Meshlets.size(),
		reinterpret_cast<const uint16_t*>(this->UniqueVertexIB.data()), this->UniqueVertexIB.size() / sizeof(uint16_t),
		this->MeshletTriangles.data(), this->MeshletTriangles.size(),
		this->MeshletCullData);

}

void PhxEngine::Scene::MeshComponent::ComputeBounds()
{
	DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(Math::cMaxFloat, Math::cMaxFloat, Math::cMaxFloat);
	DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(Math::cMinFloat, Math::cMinFloat, Math::cMinFloat);

	if (this->Positions)
	{
		for (int i = 0; i < this->TotalVertices; i++)
		{
			minBounds = Math::Min(minBounds, this->Positions[i]);
			maxBounds = Math::Max(maxBounds, this->Positions[i]);
		}
	}

	this->Aabb = AABB(minBounds, maxBounds);
	this->BoundingSphere = Sphere(minBounds, maxBounds);
}
