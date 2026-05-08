#include "PhxResourceCompiler_pch.h"

#include <PhxResourceCompiler/Mesh/MeshSerializer.h>
#include <PhxResourceCompiler/Mesh/MeshTypes.h>

#include <PhxCore/BinaryBuilder.h>

#include <PhxCore/BinaryBuilder.h>

#include <PhxResource/ResourceFileFormat.h>

#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/MeshResource.h>

using namespace phx;
using namespace phx::resource;
using namespace phx::renderer;

Result<phx::MemoryBuffer> phx::resource::serializer::SerializeMesh(const compiler::BakedMesh &mesh)
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
	const size_t cpu_chunk_data_size =  cpu_data_size + sizeof(MeshResource::CpuData::Draw) *mesh.sub_meshes.size();
	OffsetHandle cpu_chunk_heap_offset = file_builder.Reserve(cpu_chunk_data_size, 16u);

	const size_t gpu_chunk_data_size = mesh.vertex_buffer.Size() +mesh.index_buffer.Size();
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
		cpu_chunk_data->bounding_sphere[0] = mesh.bounds_ls.centre.x;
		cpu_chunk_data->bounding_sphere[1] = mesh.bounds_ls.centre.y;
		cpu_chunk_data->bounding_sphere[2] = mesh.bounds_ls.centre.z;
		cpu_chunk_data->bounding_sphere[3] = mesh.bounds_ls.radius;

		cpu_chunk_data->bounding_box[0] = mesh.bbox_ls.min.x;
		cpu_chunk_data->bounding_box[1] = mesh.bbox_ls.min.y;
		cpu_chunk_data->bounding_box[2] = mesh.bbox_ls.min.z;
		cpu_chunk_data->bounding_box[3] = mesh.bbox_ls.max.x;
		cpu_chunk_data->bounding_box[4] = mesh.bbox_ls.max.y;
		cpu_chunk_data->bounding_box[5] = mesh.bbox_ls.max.z;

		cpu_chunk_data->index_data_offset = 0;
		cpu_chunk_data->index_data_size = static_cast<uint32_t>(mesh.index_buffer.Size());

		cpu_chunk_data->vertex_data_offset = static_cast<uint32_t>(mesh.index_buffer.Size());
		cpu_chunk_data->vertex_data_size = static_cast<uint32_t>(mesh.vertex_buffer.Size());

		cpu_chunk_data->num_draws = static_cast<uint32_t>(mesh.sub_meshes.size());

		// Retrive a pointer to the off
		void* draws_ptr = reinterpret_cast<std::byte*>(cpu_chunk_data) + cpu_data_size;
		cpu_chunk_data->draws.Set(draws_ptr);

		for (size_t i = 0 ; i <mesh.sub_meshes.size(); i++)
		{
			MeshResource::CpuData::Draw* draw = reinterpret_cast<MeshResource::CpuData::Draw*>(draws_ptr) + i;
			const compiler::BakedMesh::SubMeshView& sub_mesh = mesh.sub_meshes[i];

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
			mesh.index_buffer.Data(),
			mesh.index_buffer.Size());

		std::memcpy(
			gpu_chunk_data +mesh.index_buffer.Size(),
			mesh.vertex_buffer.Data(),mesh.vertex_buffer.Size());
	}

    return file_builder.Finalize();
}