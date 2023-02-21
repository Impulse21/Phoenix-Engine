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

void MeshComponent::CreateRenderData(
	RHI::ICommandList* commandList,
	Renderer::ResourceUpload& indexUploader,
	Renderer::ResourceUpload& vertexUploader)
{
	// Construct the Mesh buffer
	if (!this->Indices.empty() && !this->IndexGpuBuffer.IsValid())
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = this->GetIndexBufferSizeInBytes();
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexGpuBuffer = RHI::IGraphicsDevice::GPtr->CreateIndexBuffer(indexBufferDesc);

		auto offset = indexUploader.SetData(this->Indices.data(), indexBufferDesc.SizeInBytes);

		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->CopyBuffer(
			this->IndexGpuBuffer,
			0,
			indexUploader.UploadBuffer,
			offset,
			indexBufferDesc.SizeInBytes);
		commandList->TransitionBarrier(
			this->IndexGpuBuffer,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource);
	}

	// Construct the Vertex Buffer
	// Set up the strides and offsets
	RHI::BufferDesc vertexDesc = {};
	if (!this->VertexGpuBuffer.IsValid())
	{
		RHI::BufferDesc vertexDesc = {};
		vertexDesc.StrideInBytes = sizeof(float);
		vertexDesc.DebugName = "Vertex Buffer";
		vertexDesc.EnableBindless();
		vertexDesc.IsRawBuffer();
		vertexDesc.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource;
		vertexDesc.SizeInBytes = this->GetVertexBufferSizeInBytes();

		// Is this Needed for Raw Buffer Type
		this->VertexGpuBuffer = RHI::IGraphicsDevice::GPtr->CreateVertexBuffer(vertexDesc);

		std::vector<uint8_t> gpuBufferData(vertexDesc.SizeInBytes);
		std::memset(gpuBufferData.data(), 0, vertexDesc.SizeInBytes);
		uint64_t bufferOffset = 0ull;

		uint64_t startOffset = vertexUploader.Offset;

		auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* Data, uint64_t sizeInBytes)
		{
			auto& bufferRange = this->GetVertexAttribute(attr);
			bufferRange.ByteOffset = bufferOffset;
			bufferRange.SizeInBytes = sizeInBytes;
			bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, kVertexBufferAlignment);
			vertexUploader.SetData(Data, bufferRange.SizeInBytes, kVertexBufferAlignment);
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
		commandList->CopyBuffer(
			this->VertexGpuBuffer,
			0,
			vertexUploader.UploadBuffer,
			startOffset,
			vertexDesc.SizeInBytes);
		commandList->TransitionBarrier(this->VertexGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
	}

	// Construct AABB
	// TODO: Experiment with SIMD
	{
		DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(Math::cMaxFloat, Math::cMaxFloat, Math::cMaxFloat);
		DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(Math::cMinFloat, Math::cMinFloat, Math::cMinFloat);

		if (!this->VertexPositions.empty())
		{
			for (const DirectX::XMFLOAT3& pos : this->VertexPositions)
			{
				minBounds = Math::Min(minBounds, pos);
				maxBounds = Math::Max(maxBounds, pos);
			}
		}

		this->Aabb = AABB(minBounds, maxBounds);
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
	DirectX::ComputeMeshlets(
		this->Indices.data(), this->Indices.size() / 3,
		this->VertexPositions.data(), this->VertexPositions.size(),
		subsets.data(), subsets.size(),
		nullptr,
		meshlets,
		uniqueVertexIB,
		prims,
		meshletSubsets.data(),
		128u,
		MESHLET_MAXIMUM_SIZE);

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

		const size_t sizeInBytes = uniqueVertexIB.size() * sizeof(uint8_t);
		this->UniqueVertexIndices = gfxDevice->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
				.Binding = RHI::BindingFlags::ShaderResource,
				.InitialState = RHI::ResourceStates::CopyDest,
				.StrideInBytes = sizeof(uint8_t),
				.SizeInBytes = sizeInBytes,
				.DebugName = "Unique Vertex IB",
			});

		Renderer::ResourceUpload unqiueVertexIBUploader = Renderer::CreateResourceUpload(sizeInBytes);

		auto offset = unqiueVertexIBUploader.SetData(uniqueVertexIB.data(), uniqueVertexIB.size() * sizeof(uint8_t));

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

uint64_t PhxEngine::Scene::MeshComponent::GetIndexBufferSizeInBytes() const
{
	return sizeof(uint32_t) * this->Indices.size();
}

uint64_t PhxEngine::Scene::MeshComponent::GetVertexBufferSizeInBytes() const
{
	return 
		Helpers::AlignTo(this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment) +
		Helpers::AlignTo(this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2), kVertexBufferAlignment) +
		Helpers::AlignTo(this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment) +
		Helpers::AlignTo(this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4), kVertexBufferAlignment) +
		Helpers::AlignTo(this->VertexColour.size() * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);
}
