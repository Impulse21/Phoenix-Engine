#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Scene/Components.h>

#include <PhxEngine/Core/Helpers.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;


void MeshComponent::CreateRenderData(RHI::CommandListHandle commandList)
{
	// Construct the Mesh buffer
	if (!this->Indices.empty() && !this->IndexGpuBuffer.IsValid())
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = sizeof(uint32_t) * this->Indices.size();
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexGpuBuffer = RHI::IGraphicsDevice::Ptr->CreateIndexBuffer(indexBufferDesc);

		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer<uint32_t>(this->IndexGpuBuffer, this->Indices);
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

		const uint64_t alignment = 16ull;
		vertexDesc.SizeInBytes =
			Helpers::AlignTo(this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3), alignment) +
			Helpers::AlignTo(this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2), alignment) +
			Helpers::AlignTo(this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3), alignment) +
			Helpers::AlignTo(this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4), alignment) +
			Helpers::AlignTo(this->VertexColour.size() * sizeof(DirectX::XMFLOAT3), alignment);

		// Is this Needed for Raw Buffer Type
		this->VertexGpuBuffer = RHI::IGraphicsDevice::Ptr->CreateIndexBuffer(vertexDesc);

		std::vector<uint8_t> gpuBufferData(vertexDesc.SizeInBytes);
		std::memset(gpuBufferData.data(), 0, vertexDesc.SizeInBytes);
		uint64_t bufferOffset = 0ull;

		auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* data, uint64_t sizeInBytes)
		{
			auto& bufferRange = this->GetVertexAttribute(attr);
			bufferRange.ByteOffset = bufferOffset;
			bufferRange.SizeInBytes = sizeInBytes;

			bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, alignment);
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

	// Create RT BLAS object
	if (RHI::IGraphicsDevice::Ptr->CheckCapability(RHI::DeviceCapability::RayTracing))
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
			geometry.Triangles.VertexFormat = RHI::FormatType::RGB32_FLOAT;

			geometry.Triangles.IndexBuffer = this->IndexGpuBuffer;
			geometry.Triangles.IndexCount = surface.NumIndices;
			geometry.Triangles.IndexOffset = surface.IndexOffsetInMesh;
		}

		this->Blas = RHI::IGraphicsDevice::Ptr->CreateRTAccelerationStructure(rtDesc);
	}
}