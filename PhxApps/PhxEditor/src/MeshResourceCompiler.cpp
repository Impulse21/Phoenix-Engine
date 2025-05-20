#include "MeshResourceCompiler.h"

#include "PhxCore/Span.h"
#include "PhxCore/BinaryBuilder.h"

#include "PhxRhi/RHITypes.h"

#include "PhxResource/ResourceFileFormat.h"
#include "PhxRenderer/MeshResourceHandler.h"

#include <DirectXMath.h>

using namespace phxed;
using namespace phx;
using namespace phx::renderer;

void phxed::MeshResourceCompiler::Compile()
{
	m_outCompiledResource.Name = m_meshData.Name;
	m_outCompiledResource.Ext = ResourceExtension< renderer::MeshResourceHandler>::value;

	std::vector<uint8_t> gpuData;
	BuildGpuBufferData(gpuData);
	// metadata chunk
	{
		auto metadata = static_cast<renderer::MeshMetadata*>(malloc(sizeof(MeshMetadata)));
		assert(metadata);
		std::memset(metadata, 0, sizeof(MeshMetadata));

		metadata->GeometryBufferSize = static_cast<uint32_t>(gpuData.size());
		metadata->VertexBufferOffset = m_meshData.Indices.size() * sizeof(uint32_t);
		m_outCompiledResource.MetadataChunk = std::make_unique<Blob>(metadata, sizeof(MeshMetadata));
	}

	{
		const size_t cpuDataSize = sizeof(renderer::MeshResource::CpuData) + (sizeof(renderer::MeshResource::CpuData::DrawInfo) * m_meshData.Geometry.size());
		auto cpuData = reinterpret_cast<renderer::MeshResource::CpuData*>(malloc(cpuDataSize));
		cpuData->IbSize = m_meshData.Indices.size() * sizeof(uint32_t);
		cpuData->IbOffset = 0;
		cpuData->IbFormat = 0;
		cpuData->VbOffset = m_meshData.Indices.size() * sizeof(uint32_t);
		cpuData->VbSize = gpuData.size() - cpuData->IbSize;
		cpuData->NumDraws = static_cast<uint16_t>(m_meshData.Geometry.size());
		for (size_t i = 0; i < m_meshData.Geometry.size(); i++)
		{
			const MeshData::GeometryData& srcGeo = m_meshData.Geometry[i];
			MeshResource::CpuData::DrawInfo& drawInfo = *(cpuData->Draw + i);
			drawInfo.IndexCount = srcGeo.IndexCount;
			drawInfo.StartIndex = srcGeo.IndexOffset;
			drawInfo.BaseVertex = 0;
		}

		m_outCompiledResource.Chunks.emplace_back(std::make_unique<Blob>(cpuData, cpuDataSize));
	}

	{
		void* gpuDataDest = malloc(gpuData.size());
		std::memcpy(gpuDataDest, gpuData.data(), gpuData.size());

		m_outCompiledResource.Chunks.emplace_back(std::make_unique<Blob>(gpuDataDest, gpuData.size()));
	}
}

void phxed::MeshResourceCompiler::BuildGpuBufferData(std::vector<uint8_t>& gpuBuffer) const
{
	gpuBuffer.resize(m_meshData.Indices.size() * sizeof(uint32_t));
	memcpy(
		gpuBuffer.data(),
		m_meshData.Indices.data(),
		m_meshData.Indices.size() * sizeof(uint32_t));

	BuildVertexBuffer(gpuBuffer);
}

void phxed::MeshResourceCompiler::BuildVertexBuffer(std::vector<uint8_t>& gpuBuffer) const
{
	// TODO: I think this can be written in a more eligent way
	// such that it's easier to expand to new stream enums without needing to manually
	// add new entires. Will deffer to a later time as I wont to get things
	// working for now.

	PHX_ASSERT(m_meshData.GetVertexStream(phx::renderer::VertexStream_Position));

	BinaryBuilder vbBuilder;
	OffsetHandle headerOffset = vbBuilder.Reserve<renderer::VertexStreamsHeader>();

	std::array<OffsetHandle, renderer::VertexStream_Count> streamOffsets;
	std::memset(streamOffsets.data(), 0xFF, sizeof(OffsetHandle) * renderer::VertexStream_Count);

	for (auto& streamOpt : m_meshData.VertexStreams)
	{
		if (!streamOpt.has_value())
			continue;
		
		const VertexStream& stream = streamOpt.value();
		const std::size_t sizeInBytes = stream.ElementStride * stream.NumElements;
		streamOffsets[stream.Type] = vbBuilder.Reserve(sizeInBytes, 16u);
	}

	vbBuilder.Commit();
	
	// Fill in the data.
	auto header = vbBuilder.Place<renderer::VertexStreamsHeader>(headerOffset);
	for (auto& streamOpt : m_meshData.VertexStreams)
	{
		if (!streamOpt.has_value())
			continue;

		const VertexStream& stream = streamOpt.value();
		header->Desc[stream.Type].SetOffset((uint)streamOffsets[stream.Type]);
		header->Desc[stream.Type].SetStride((uint)stream.ElementStride);
	}

	for (auto& streamOpt : m_meshData.VertexStreams)
	{
		if (!streamOpt.has_value())
			continue;

		const VertexStream& stream = streamOpt.value();
		const std::size_t sizeInBytes = stream.ElementStride * stream.NumElements;

		auto* data = vbBuilder.Place(streamOffsets[stream.Type]);
		std::memcpy(
			data,
			stream.Data.get(),
			sizeInBytes);
	}

	phx::Span<uint8_t> memory = vbBuilder.GetMemory();

	const size_t vbOffset = gpuBuffer.size();
	gpuBuffer.resize(vbOffset + memory.size());

	memcpy(
		gpuBuffer.data() + vbOffset,
		memory.data(),
		memory.size());
}
