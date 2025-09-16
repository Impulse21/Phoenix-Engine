#include "ModelExporter.h"

#include "PhxRenderer/ModelResoure.h"
#include <PhxCore/BinaryBuilder.h>

using namespace phx;
using namespace phx::renderer;
void ModelExporter::Export()
{
	// metadata chunk
	{
		m_compiled_resource.metadata_chunk = MemoryBuffer::Create<ModelMetadata>();

		auto metadata_view = m_compiled_resource.metadata_chunk.GetView<ModelMetadata>();
		std::memset(metadata_view.Get(), 0, m_compiled_resource.metadata_chunk.Size());

		metadata_view->geometry_bufer_size = static_cast<uint32_t>(m_model_data.geometry_data.size());

		{
			BinaryBuilder cpu_data_builder = {};

			// fill in data
			
			// TODO: I am here
			const size_t cpu_data_size = 
				sizeof(renderer::mesh::CpuData) + 
				(sizeof(renderer::MeshResource::CpuData::DrawInfo) * m_model_data.meshes.size());

			MemoryBuffer& cpu_chunk_data = m_compiled_resource.chunks.emplace_back<MemoryBuffer>(MemoryBuffer(cpu_data_size));
			auto cpu_chunk_view = cpu_chunk_data.GetView<renderer::MeshResource::CpuData>();

			cpu_chunk_view->ib_size = 0;
			cpu_chunk_view->vb_offset = 0;
			cpu_chunk_view->vb_size = 0;
			cpu_chunk_view->num_draws = static_cast<uint16_t>(m_model_data.meshes.Geometry.size());
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
