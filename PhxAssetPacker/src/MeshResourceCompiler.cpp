#include "MeshResourceCompiler.h"

#include "PhxCore/Span.h"
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

	renderer::MeshCpuMetadata& metadata = m_outCompiledMesh.Metadata;
	metadata.IbOffset = 0;
	metadata.IbFormat = static_cast<uint8_t>(rhi::Format::R32_UINT);
	metadata.IbSize = m_meshData.Indices.size();

	// set index data
	std::vector<uint8_t>& gpuData = m_outCompiledMesh.GpuData;
	gpuData.resize(m_meshData.Indices.size() * sizeof(uint32_t));

	metadata.VbOffset = 0;
	metadata.VbSize = gpuData.size();
	memcpy(
		gpuData.data(),
		m_meshData.Indices.data(),
		m_meshData.Indices.size() * sizeof(uint32_t));


	BuildVertexBuffer(gpuData);	

	// Set up draw calls
	m_outCompiledMesh.Metadata.NumDraws = m_meshData.Geometry.size();
	m_outCompiledMesh.Draws.resize(m_meshData.Geometry.size());
	for (size_t i = 0; i < m_meshData.Geometry.size(); i++)
	{
		const MeshData::GeometryData& geometry = m_meshData.Geometry[i];
		MeshCpuMetadata::DrawInfo& drawInfo = m_outCompiledMesh.Draws[i];
		drawInfo.IndexCount = geometry.IndexCount;
		drawInfo.StartIndex = geometry.IndexOffset;
		drawInfo.BaseVertex = 0;
	}

}

void phx::MeshResourceCompiler::BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer)
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
	m_outCompiledMesh.Metadata.VbSize = memory.size();

	const size_t vbOffset = gpuBuffer.size();
	gpuBuffer.resize(vbOffset + memory.size());
	
	memcpy(
		gpuBuffer.data() + vbOffset,
		memory.data(),
		memory.size());
}
