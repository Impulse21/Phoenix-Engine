#include <PhxRenderer/PhxRenderer_pch.h>

#include "IntermediateMeshExporter.h"
#include "IntermediateMesh.h"

#include <PhxCore/BinaryBuilder.h>

#include <PhxResource/ResourceFileFormat.h>

#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/MeshResource.h>

using namespace phx::renderer;
using namespace phx::renderer::compiler;

IntermediateMeshExporter::IntermediateMeshExporter(IntermediateMesh const& intermediate_mesh, std::ostream& out)
	: m_intermediate_mesh(intermediate_mesh)
	, m_out(out)
{
}

bool IntermediateMeshExporter::operator()()
{
	BinaryBuilder file_builder;

	const OffsetHandle header_offset = file_builder.Reserve<ResourceFileFormat::Header>();
	const OffsetHandle metadata_header_offset = file_builder.Reserve<ResourceFileFormat::MetadataHeader>();


	// Mesh Resources have two chunks. CPU and GPU.
	// Future pass will add a chunks for meshlet data.
	const size_t chunk_count = 2;
	const OffsetHandle chunk_header_offset = file_builder.ReserveArray<ResourceFileFormat::Chunk>(chunk_count);

	const size_t metadata_heap_size = file_builder.GetSize() - metadata_header_offset;

	const size_t cpu_data_size = AlignUp(sizeof(MeshResource::CpuData), 16u);
	const size_t cpu_chunk_data_size =  cpu_data_size + sizeof(MeshResource::CpuData::Draw) * m_intermediate_mesh.sub_meshes.size();
	OffsetHandle cpu_chunk_heap_offset = file_builder.Reserve(cpu_chunk_data_size, 16u);

	const size_t gpu_chunk_data_size = m_intermediate_mesh.vertex_buffer.Size() + m_intermediate_mesh.index_buffer.Size();
	OffsetHandle gpu_chunk_heap_offset = file_builder.Reserve(gpu_chunk_data_size, 16u);

	file_builder.Commit();
	{
		auto* header = file_builder.PlaceType<ResourceFileFormat::Header>(header_offset);

		header->Magic = ResourceFileFormat::MagicNumber;
		header->Version = ResourceFileFormat::Version;
		header->BuildNumber = FileFormat::GetTimestamp();
		header->HandlerId = 0;
		header->MetadataHeapSize = metadata_heap_size;
	}

	auto* metdata_header = file_builder.PlaceType<ResourceFileFormat::MetadataHeader>(metadata_header_offset);
	{
		metdata_header->NumChunks = chunk_count;
		metdata_header->NumStrings = 0;
	}

	{
		auto* chunk_headers = file_builder.PlaceType<ResourceFileFormat::Chunk>(chunk_header_offset);
		metdata_header->Chunks.Set(chunk_headers);

		// -- CPU Chunk ---
		chunk_headers[0].Compression = FileFormat::CompressionType::None;
		chunk_headers[0].Offset.Offset = cpu_chunk_heap_offset;
		chunk_headers[0].UncompressedSize = cpu_chunk_data_size;
		chunk_headers[0].CompressedSize = chunk_headers[0].UncompressedSize;

		// -- GPU Chunk ---
		chunk_headers[1].Compression = FileFormat::CompressionType::None;
		chunk_headers[1].Offset.Offset = gpu_chunk_heap_offset;
		chunk_headers[1].UncompressedSize = gpu_chunk_data_size;
		chunk_headers[1].CompressedSize = chunk_headers[1].UncompressedSize;
	}

	// -- Cpu Chunk Data ---
	{
		auto* cpu_chunk_data = file_builder.PlaceType<MeshResource::CpuData>(cpu_chunk_heap_offset);
		cpu_chunk_data->bounding_sphere[0] = m_intermediate_mesh.bounds_ls.centre.x;
		cpu_chunk_data->bounding_sphere[1] = m_intermediate_mesh.bounds_ls.centre.y;
		cpu_chunk_data->bounding_sphere[2] = m_intermediate_mesh.bounds_ls.centre.z;
		cpu_chunk_data->bounding_sphere[3] = m_intermediate_mesh.bounds_ls.radius;

		cpu_chunk_data->bounding_box[0] = m_intermediate_mesh.bbox_ls.min.x;
		cpu_chunk_data->bounding_box[1] = m_intermediate_mesh.bbox_ls.min.y;
		cpu_chunk_data->bounding_box[2] = m_intermediate_mesh.bbox_ls.min.z;
		cpu_chunk_data->bounding_box[3] = m_intermediate_mesh.bbox_ls.max.x;
		cpu_chunk_data->bounding_box[4] = m_intermediate_mesh.bbox_ls.max.y;
		cpu_chunk_data->bounding_box[5] = m_intermediate_mesh.bbox_ls.max.z;

		cpu_chunk_data->index_data_offset = 0;
		cpu_chunk_data->index_data_size = static_cast<uint32_t>(m_intermediate_mesh.index_buffer.Size());

		cpu_chunk_data->vertex_data_offset = static_cast<uint32_t>(m_intermediate_mesh.index_buffer.Size());
		cpu_chunk_data->vertex_data_size = static_cast<uint32_t>(m_intermediate_mesh.vertex_buffer.Size());

		cpu_chunk_data->num_draws = static_cast<uint32_t>(m_intermediate_mesh.sub_meshes.size());

		// Retrive a pointer to the off
		void* draws_ptr = reinterpret_cast<std::byte*>(cpu_chunk_data) + cpu_data_size;
		cpu_chunk_data->draws.Set(draws_ptr);

		for (size_t i = 0 ; i < m_intermediate_mesh.sub_meshes.size(); i++)
		{
			MeshResource::CpuData::Draw* draw = reinterpret_cast<MeshResource::CpuData::Draw*>(draws_ptr) + i;
			const IntermediateMesh::SubMeshView& sub_mesh = m_intermediate_mesh.sub_meshes[i];

			draw->prim_count = sub_mesh.index_count;
			// TODO: clear this up to match what is required when drawing.
			draw->start_index = sub_mesh.index_offset;
			draw->base_vertex = sub_mesh.vertex_offset;
		}
	}
	// -- Gpu Chunk Data ---
	{
		std::byte* gpu_chunk_data = reinterpret_cast<std::byte*>(file_builder.Place(gpu_chunk_heap_offset));
		std::memcpy(
			gpu_chunk_data,
			m_intermediate_mesh.index_buffer.Data(),
			m_intermediate_mesh.index_buffer.Size());

		std::memcpy(
			gpu_chunk_data + m_intermediate_mesh.index_buffer.Size(),
			m_intermediate_mesh.vertex_buffer.Data(), m_intermediate_mesh.vertex_buffer.Size());
	}

	MemoryBuffer file_data_buffer = file_builder.Finalize();
	m_out.write(reinterpret_cast<const char*>(file_data_buffer.Data()), file_data_buffer.Size());

	return true;
}