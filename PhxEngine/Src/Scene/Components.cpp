#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>

#include <PhxEngine/Core/Helpers.h>
#include <DirectXMesh.h>
#include <PhxEngine/Renderer/Renderer.h>


using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

constexpr uint64_t kVertexBufferAlignment = 16ull;

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
	RHI::ICommandList* commandList)
{
	// Construct the Mesh buffer
	if (this->Indices && !this->IndexBuffer.IsValid())
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = sizeof(uint32_t) * TotalIndices;
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexBuffer = RHI::IGraphicsDevice::GPtr->CreateIndexBuffer(indexBufferDesc);

		commandList->TransitionBarrier(this->IndexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(this->IndexBuffer, this->Indices);

		commandList->TransitionBarrier(
			this->IndexBuffer,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource);
	}

	Renderer::ResourceUpload vertexUpload = {};
	if (this->Positions && !this->IndexBuffer.IsValid())
	{
		size_t vertexBufferSize = this->TotalVertices * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment;
		if (this->Normals && (this->Flags & MeshComponent::Flags::kContainsNormals) == 1)
		{
			vertexBufferSize += this->TotalVertices * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment;
		}

		if (this->TexCoords && (this->Flags & MeshComponent::Flags::kContainsTexCoords) == 1)
		{
			vertexBufferSize += this->TotalVertices * sizeof(DirectX::XMFLOAT2), kVertexBufferAlignment;
		}

		if (this->Tangents && (this->Flags & MeshComponent::Flags::kContainsTangents) == 1)
		{
			vertexBufferSize += this->TotalVertices * sizeof(DirectX::XMFLOAT4), kVertexBufferAlignment;
		}

		RHI::BufferDesc vertexDesc = {};
		vertexDesc.StrideInBytes = sizeof(float);
		vertexDesc.DebugName = "Vertex Buffer";
		vertexDesc.EnableBindless();
		vertexDesc.IsRawBuffer();
		vertexDesc.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource;
		vertexDesc.SizeInBytes = vertexBufferSize;

		// Is this Needed for Raw Buffer Type
		this->VertexBuffer = RHI::IGraphicsDevice::GPtr->CreateVertexBuffer(vertexDesc);
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
				this->TexCoords.data(),
				this->TotalVertices * sizeof(DirectX::XMFLOAT2));
		}

		if (this->Normals)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Normal,
				this->VertexNormals.data(),
				this->TotalVertices * sizeof(DirectX::XMFLOAT3));
		}

		if (this->Tangents)
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Tangent,
				this->Tangents,
				this->TotalVertices * sizeof(DirectX::XMFLOAT4));
		}

		if (this->Colour)
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

	// Construct AABB
	// TODO: Experiment with SIMD
	{
	}

	// Create RT BLAS object
	if (RHI::IGraphicsDevice::GPtr->CheckCapability(RHI::DeviceCapability::RayTracing))
	{
		this->BlasState = BLASState::Rebuild;

		// Create Blast
		RHI::RTAccelerationStructureDesc rtDesc = {};
		rtDesc.Type = RHI::RTAccelerationStructureDesc::Type::BottomLevel;
		rtDesc.Flags == RHI::RTAccelerationStructureDesc::kPreferFastTrace;

		for (int i = 0; i < this->Surfaces.size(); i++)
		{
			auto& surface = this->Surfaces[i];

			auto& geometry = rtDesc.ButtomLevel.Geometries.emplace_back();
			geometry.Type = RHI::RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles;
			geometry.Triangles.VertexBuffer = this->VertexGpuBuffer;
			geometry.Triangles.VertexStride = sizeof(DirectX::XMFLOAT3);
			geometry.Triangles.VertexByteOffset = 0;
			geometry.Triangles.VertexCount = (uint32_t)this->VertexPositions.size();
			geometry.Triangles.VertexFormat = RHI::RHIFormat::RGB32_FLOAT;

			geometry.Triangles.IndexBuffer = this->IndexGpuBuffer;
			geometry.Triangles.IndexCount = surface.NumIndices;
			geometry.Triangles.IndexOffset = surface.IndexOffsetInMesh;
		}

		this->Blas = RHI::IGraphicsDevice::GPtr->CreateRTAccelerationStructure(rtDesc);
	}
}

void PhxEngine::Scene::MeshComponent::ComputeMeshlets(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList)
{
	std::vector<uint32_t> attributes;
	for (int i = 0; i < this->Surfaces.size(); i++)
	{
		auto& surface = this->Surfaces[i];
		for (int j = surface.IndexOffsetInMesh; j < surface.NumIndices; j +=3)
		{
			attributes.emplace_back(i);
		}
	}

	auto subsets = DirectX::ComputeSubsets(attributes.data(), attributes.size());
	this->indexSubsets.resize(subsets.size());
	for (uint32_t i = 0; i < subsets.size(); ++i)
	{
		indexSubsets[i].Offset = static_cast<uint32_t>(subsets[i].first) * 3;
		indexSubsets[i].Count = static_cast<uint32_t>(subsets[i].second) * 3;
	}

	std::vector<std::pair<size_t, size_t>> meshletSubsets(subsets.size());
	std::vector<DirectX::Meshlet> meshlets;
	std::vector<uint8_t> uniqueVertexIB;
	std::vector<DirectX::MeshletTriangle> prims;


	this->MeshletSubsets.resize(meshletSubsets.size());
	for (uint32_t i = 0; i < meshletSubsets.size(); ++i)
	{
		this->MeshletSubsets[i].Offset = static_cast<uint32_t>(meshletSubsets[i].first);
		this->MeshletSubsets[i].Count = static_cast<uint32_t>(meshletSubsets[i].second);
	}

	{
		this->MeshletsCount = meshlets.size();
		const size_t sizeInBytes = meshlets.size() * sizeof(DirectX::Meshlet);
		this->Meshlets = gfxDevice->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.InitialState = RHI::ResourceStates::CopyDest,
				.StrideInBytes = sizeof(DirectX::Meshlet),
				.SizeInBytes = sizeInBytes,
				.DebugName = "Meshlet Buffer",
			});


		Renderer::ResourceUpload meshletsUploader = Renderer::CreateResourceUpload(sizeInBytes);

		auto offset = meshletsUploader.SetData(meshlets.data(), meshlets.size() * sizeof(DirectX::Meshlet));
		commandList->CopyBuffer(
			this->Meshlets,
			0,
			meshletsUploader.UploadBuffer,
			offset,
			sizeInBytes);

		commandList->TransitionBarrier(
			this->Meshlets,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::ShaderResource);

		meshletsUploader.Free();
	}

	{
		const size_t sizeInBytes = uniqueVertexIB.size();
		this->UniqueVertexIndices = gfxDevice->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.InitialState = RHI::ResourceStates::CopyDest,
				.SizeInBytes = sizeInBytes,
				.DebugName = "Unique Vertex IB",
			});

		Renderer::ResourceUpload unqiueVertexIBUploader = Renderer::CreateResourceUpload(sizeInBytes);

		auto offset = unqiueVertexIBUploader.SetData(uniqueVertexIB.data(), uniqueVertexIB.size());

		commandList->CopyBuffer(
			this->UniqueVertexIndices,
			0,
			unqiueVertexIBUploader.UploadBuffer,
			offset,
			sizeInBytes);

		commandList->TransitionBarrier(
			this->UniqueVertexIndices,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::ShaderResource);

		unqiueVertexIBUploader.Free();
	}

	{
		const size_t sizeInBytes = prims.size() * sizeof(DirectX::MeshletTriangle);
		this->PrimitiveIndices = gfxDevice->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.InitialState = RHI::ResourceStates::CopyDest,
				.StrideInBytes = sizeof(DirectX::MeshletTriangle),
				.SizeInBytes = sizeInBytes,
				.DebugName = "Primitive Indices",
			});

		Renderer::ResourceUpload meshletTriangleUplaoder = Renderer::CreateResourceUpload(sizeInBytes);

		auto offset = meshletTriangleUplaoder.SetData(prims.data(), prims.size() * sizeof(DirectX::MeshletTriangle));

		commandList->CopyBuffer(
			this->PrimitiveIndices,
			0,
			meshletTriangleUplaoder.UploadBuffer,
			offset,
			sizeInBytes);

		commandList->TransitionBarrier(
			this->PrimitiveIndices,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::ShaderResource);

		meshletTriangleUplaoder.Free();
	}
}
