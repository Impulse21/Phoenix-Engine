#include "MeshResourceCompiler.h"

#include "PhxCore/Span.h"
#include "PhxCore/IO/ChunkFile.h"

#include "PhxRhi/RHITypes.h"

#include "PhxCore/BinaryBuilder.h"
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
	std::memset(&m_outCompiledMesh, 0, sizeof(CompiledMeshResource));

	BinaryBuilder chunkFileBuilder;
	OffsetHandle headerOffset = chunkFileBuilder.Reserve<ChunkFileFormat::Header>();
	OffsetHandle cpuChunkHeaderOffset = chunkFileBuilder.Reserve<ChunkFileFormat::ChunkHeader>();
	OffsetHandle gpuChunkHeaderOffset = chunkFileBuilder.Reserve<ChunkFileFormat::ChunkHeader>();

	// Determine the size of the CPU metdata based on the number of draw calls
	const size_t cpuDataSize = sizeof(renderer::MeshCpuMetadata) + (sizeof(renderer::MeshCpuMetadata) * m_meshData.Geometry.size() - 1);
	OffsetHandle cpuDataOffset = chunkFileBuilder.Reserve(cpuDataSize, 4ull);

	// set index data
	std::vector<uint8_t> gpuData;
	BuildGpuBufferData(gpuData);
	OffsetHandle gpuDataOffset = chunkFileBuilder.Reserve(gpuData.size(), 16ull);

	// Fill in the data
	chunkFileBuilder.Commit();
	{
		auto& header = *chunkFileBuilder.Place<ChunkFileFormat::Header>(headerOffset);
		header.Magic = MakeMagicNum('P', 'X', 'M', 'S');
		header.Version = MshVersion;
		header.BuildNumber = GetTimestamp();
		header.ChunkCount = 2; // Two Chunks
	}

	{
		auto& cpuMetadataHeader = *chunkFileBuilder.Place<ChunkFileFormat::ChunkHeader>(cpuChunkHeaderOffset);
		cpuMetadataHeader.ChunkID = MakeMagicNum('M', 'E', 'T', 'A');
		cpuMetadataHeader.Compression = CompressionType::None;
		cpuMetadataHeader.UncompressedSize = cpuDataOffset;
		cpuMetadataHeader.CompressedSize = cpuMetadataHeader.UncompressedSize;
		cpuMetadataHeader.Offset = static_cast<uint32_t>(cpuDataOffset);
		
		auto& gpuHeader = *chunkFileBuilder.Place<ChunkFileFormat::ChunkHeader>(gpuChunkHeaderOffset);
		gpuHeader.ChunkID = MakeMagicNum('G', 'B', 'U', 'F');

		// TODO: Compress
		gpuHeader.Compression = CompressionType::None;
		gpuHeader.UncompressedSize = gpuData.size();
		gpuHeader.CompressedSize = cpuMetadataHeader.UncompressedSize;
		gpuHeader.Offset = static_cast<uint32_t>(gpuDataOffset);
	}

	// Fill in the data
	{
		auto& cpuMetadata = *chunkFileBuilder.Place<MeshCpuMetadata>(cpuDataOffset);
		std::memset(&cpuMetadata, 0, cpuDataSize);

		cpuMetadata.IbSize = m_meshData.Indices.size() * sizeof(uint32_t);
		cpuMetadata.VbOffset = m_meshData.Indices.size() * sizeof(uint32_t);
		cpuMetadata.VbSize = gpuData.size() - cpuMetadata.IbSize;
		cpuMetadata.NumDraws = m_meshData.Geometry.size();
		for (size_t i = 0; i < m_meshData.Geometry.size(); i++)
		{
			const MeshData::GeometryData& geometry = m_meshData.Geometry[i];
			MeshCpuMetadata::DrawInfo& drawInfo = *(cpuMetadata.Draw + i);
			drawInfo.IndexCount = geometry.IndexCount;
			drawInfo.StartIndex = geometry.IndexOffset;
			drawInfo.BaseVertex = 0;
		}
	}

	{
		void* gpuDataDest = chunkFileBuilder.Place(gpuDataOffset);
		std::memcpy(gpuDataDest, gpuData.data(), gpuData.size());
	}

	m_outCompiledMesh.File = chunkFileBuilder.Finialize();
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

	streamOffsets[kPosition] = vbBuilder.Reserve<DirectX::XMFLOAT3>(m_meshData.Vertex_Positions.size());

	PHX_ASSERT(!m_meshData.Vertex_Normals.empty(), "Normal generation is currently not supported");
	streamOffsets[kNormals] = vbBuilder.Reserve<DirectX::XMFLOAT3>(m_meshData.Vertex_Normals.size());
	
	if (!m_meshData.Vertex_Uvset_0.empty())
	{
		streamOffsets[kUV0] = vbBuilder.Reserve<DirectX::XMFLOAT2>(m_meshData.Vertex_Uvset_0.size());
	}

	if (!m_meshData.Vertex_Uvset_1.empty())
	{
		streamOffsets[kUV1] = vbBuilder.Reserve<DirectX::XMFLOAT2>(m_meshData.Vertex_Uvset_1.size());
	}

	if (!m_meshData.Vertex_Tangents.empty())
	{
		streamOffsets[kTangents] = vbBuilder.Reserve<DirectX::XMFLOAT4>(m_meshData.Vertex_Tangents.size());
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
	FillStreamDesc(kColour, sizeof(DirectX::XMFLOAT3));
	FillStreamDesc(kJoints, sizeof(DirectX::XMFLOAT4));
	FillStreamDesc(kWeights, sizeof(DirectX::XMFLOAT4));


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
