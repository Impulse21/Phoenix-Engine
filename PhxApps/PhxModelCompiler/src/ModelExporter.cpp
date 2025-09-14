#include "ModelExporter.h"

#include "PhxRenderer/MeshResource.h"

using namespace phx;
using namespace phx::renderer;
void ModelExporter::Export()
{
	// metadata chunk
	{
		m_compiled_resource.metadata_chunk = MemoryBuffer(sizeof(MeshMetadata));

		auto metadata_view = m_compiled_resource.metadata_chunk.GetView<MeshMetadata>();
		std::memset(metadata_view.Get(), 0, sizeof(MeshMetadata));

		metadata_view->geometry_bufer_size = static_cast<uint32_t>(m_model_data.geometry_data.size());
		metadata_view->vertex_buffer_size = 0;
		{
			const size_t cpu_data_size = sizeof(renderer::MeshResource::CpuData) + (sizeof(renderer::MeshResource::CpuData::DrawInfo) * m_model_data.meshes.size());

			m_compiled_resource.chunks.emplace_back(cpu_data_size);
			auto metadata_view = m_compiled_resource.metadata_chunk.GetView<MeshMetadata>();
			auto cpuData = reinterpret_cast<renderer::MeshResource::CpuData*>(malloc(cpuDataSize));
			cpuData->IbSize = m_meshData.Indices.size() * sizeof(uint32_t);
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
