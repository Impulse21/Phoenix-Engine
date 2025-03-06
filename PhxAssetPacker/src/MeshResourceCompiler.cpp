#include "MeshResourceCompiler.h"

#include "PhxCore/Span.h"
#include "PhxCore/BinaryBuilder.h"

#include "PhxRhi/RHITypes.h"

#include "PhxResource/FileFormatUtils.h"
#include "PhxResource/ResourceFileFormat.h"
#include "PhxRenderer/MeshResourceHandler.h"

#include <DirectXMath.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
	template<typename T>
	void FillVertexBuffer(BinaryBuilder& builder, OffsetHandle offset, Span<T> srcData)
	{
		if (srcData.IsEmpty())
			return;

		auto* data = builder.Place(offset);
		std::memcpy(
			data,
			srcData.data(),
			sizeof(T) * srcData.Size());
	}
}

void phx::MeshResourceCompiler::Compile()
{
	std::memset(&m_outCompiledResource, 0, sizeof(m_outCompiledResource));

	m_outCompiledResource.Name = m_meshData.Name;
	m_outCompiledResource.Ext = ResourceExtension< renderer::MeshResourceHandler>::value;

	// Build Chunks

	std::vector<uint8_t> gpuData;
	BuildGpuBufferData(gpuData);
	// metadata chunk
	{
		auto& [id, chunk] = m_outCompiledResource.Chunks.emplace_back();
		id = ResourceFileFormat::ChunkId_Metadata;

		const size_t metadataSize = sizeof(renderer::data::MeshMetadata) + sizeof(MeshData::GeometryData) * (m_meshData.Geometry.size() - 1);

		auto metadata = reinterpret_cast<renderer::data::MeshMetadata*>(malloc(metadataSize));
		std::memset(&metadata, 0, metadataSize);

		metadata->IbSize = m_meshData.Indices.size() * sizeof(uint32_t);
		metadata->VbOffset = m_meshData.Indices.size() * sizeof(uint32_t);
		metadata->VbSize = gpuData.size() - metadata->IbSize;
		metadata->numGeometry = m_meshData.Geometry.size();
		for (size_t i = 0; i < m_meshData.Geometry.size(); i++)
		{
			const MeshData::GeometryData& srcGeo = m_meshData.Geometry[i];
			renderer::data::MeshMetadata::GeometryData& destGeo = *(metadata->Geo + i);
			destGeo.IndexCount = srcGeo.IndexCount;
			destGeo.IndexOffset = srcGeo.IndexOffset;
			destGeo.MaterialId = srcGeo.MaterialId;
		}

		chunk = std::make_unique<Blob>(metadata, metadataSize);
	}

	{
		auto& [id, chunk] = m_outCompiledResource.Chunks.emplace_back();
		id = ResourceFileFormat::ChunkId_GPUData;

		void* gpuDataDest = malloc(gpuData.size());
		std::memcpy(gpuDataDest, gpuData.data(), gpuData.size());
		chunk = std::make_unique<Blob>(gpuDataDest, gpuData.size());
	}
}

void phx::MeshResourceCompiler::BuildGpuBufferData(std::vector<uint8_t>& gpuBuffer) const
{
	gpuBuffer.resize(m_meshData.Indices.size() * sizeof(uint32_t));
	memcpy(
		gpuBuffer.data(),
		m_meshData.Indices.data(),
		m_meshData.Indices.size() * sizeof(uint32_t));

	BuildVertexBuffer(gpuBuffer);
}

void phx::MeshResourceCompiler::BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer) const
{
	// TODO: I think this can be written in a more eligent way
	// such that it's easier to expand to new stream enums without needing to manually
	// add new entires. Will deffer to a later time as I wont to get things
	// working for now.

	PHX_ASSERT(!m_meshData.Vertex_Positions.empty());

	BinaryBuilder vbBuilder;
	OffsetHandle headerOffset = vbBuilder.Reserve<renderer::VertexStreamsHeader>();

	std::array<OffsetHandle, renderer::kNumStreams> streamOffsets;
	std::memset(streamOffsets.data(), 0xFF, sizeof(OffsetHandle) * renderer::kNumStreams);

	streamOffsets[kPosition] = vbBuilder.ReserveArray<DirectX::XMFLOAT3>(m_meshData.Vertex_Positions.size());

	PHX_ASSERT(!m_meshData.Vertex_Normals.empty(), "Normal generation is currently not supported");
	streamOffsets[kNormals] = vbBuilder.ReserveArray<DirectX::XMFLOAT3>(m_meshData.Vertex_Normals.size());
	
	if (!m_meshData.Vertex_Uvset_0.empty())
	{
		streamOffsets[kUV0] = vbBuilder.ReserveArray<DirectX::XMFLOAT2>(m_meshData.Vertex_Uvset_0.size());
	}

	if (!m_meshData.Vertex_Uvset_1.empty())
	{
		streamOffsets[kUV1] = vbBuilder.ReserveArray<DirectX::XMFLOAT2>(m_meshData.Vertex_Uvset_1.size());
	}

	if (!m_meshData.Vertex_Tangents.empty())
	{
		streamOffsets[kTangents] = vbBuilder.ReserveArray<DirectX::XMFLOAT4>(m_meshData.Vertex_Tangents.size());
	}

	vbBuilder.Commit();
	
	// Fill in the data.
	auto header = vbBuilder.Place<renderer::VertexStreamsHeader>(headerOffset);

	auto FillStreamDesc = [header, &streamOffsets](VertexStreamTypes type, size_t stride) {
		header->Desc[type].SetOffset((uint)streamOffsets[type]);
		header->Desc[type].SetStride((uint)stride);
	};
	
	FillStreamDesc(kPosition, sizeof(DirectX::XMFLOAT3));
	FillStreamDesc(kNormals, sizeof(DirectX::XMFLOAT3));
	FillStreamDesc(kUV0, sizeof(DirectX::XMFLOAT2));
	FillStreamDesc(kUV1, sizeof(DirectX::XMFLOAT2));
	FillStreamDesc(kTangents, sizeof(DirectX::XMFLOAT4));
#if false
	FillStreamDesc(kColour, sizeof(DirectX::XMFLOAT3));
	FillStreamDesc(kJoints, sizeof(DirectX::XMFLOAT4));
	FillStreamDesc(kWeights, sizeof(DirectX::XMFLOAT4));
#endif

	FillVertexBuffer<DirectX::XMFLOAT3>(vbBuilder, streamOffsets[kPosition], m_meshData.Vertex_Positions);
	FillVertexBuffer<DirectX::XMFLOAT3>(vbBuilder, streamOffsets[kNormals], m_meshData.Vertex_Normals);
	FillVertexBuffer<DirectX::XMFLOAT2>(vbBuilder, streamOffsets[kUV0], m_meshData.Vertex_Uvset_0);
	FillVertexBuffer<DirectX::XMFLOAT2>(vbBuilder, streamOffsets[kUV1], m_meshData.Vertex_Uvset_1);
	FillVertexBuffer<DirectX::XMFLOAT4>(vbBuilder, streamOffsets[kTangents], m_meshData.Vertex_Tangents);

#if false
	FillVertexBuffer<DirectX::XMFLOAT3>(vbBuilder, streamOffsets[kColour], m_meshData.Vertex_Tangents);
	FillVertexBuffer<DirectX::XMFLOAT4>(vbBuilder, streamOffsets[kJoints], joints.get(), vertexCount);
	FillVertexBuffer<DirectX::XMFLOAT4>(vbBuilder, streamOffsets[kWeights], weights.get(), vertexCount);
#endif
	phx::Span<uint8_t> memory = vbBuilder.GetMemory();

	const size_t vbOffset = gpuBuffer.size();
	gpuBuffer.resize(vbOffset + memory.size());

	memcpy(
		gpuBuffer.data() + vbOffset,
		memory.data(),
		memory.size());
}
