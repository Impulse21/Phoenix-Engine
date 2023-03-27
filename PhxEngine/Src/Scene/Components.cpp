#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>

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
	/*
	// Create RT BLAS object
	if (gfxDevice->CheckCapability(RHI::DeviceCapability::RayTracing))
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

		this->Blas = gfxDevice->CreateRTAccelerationStructure(rtDesc);
	}
	*/
}