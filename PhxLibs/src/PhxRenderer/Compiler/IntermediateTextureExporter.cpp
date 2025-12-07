#include "PhxRenderer/PhxRenderer_pch.h"
#include "IntermediateTextureExporter.h"

#include <PhxCore/BinaryBuilder.h>
#include <PhxResource/ResourceFileFormat.h>

#include <PhxRenderer/TextureResourceHandler.h>
#include <PhxRenderer/TextureResource.h>

#include "IntermediateTexture.h"

using namespace phx;
using namespace phx::renderer;
using namespace phx::renderer::compiler;

bool IntermediateTextureExporter::Export(IntermediateTexture const& texture, std::ostream& out)
{
	BinaryBuilder file_builder;

	const OffsetHandle header_offset = file_builder.Reserve<ResourceFileFormat::Header>();
	const OffsetHandle metadata_header_offset = file_builder.Reserve<ResourceFileFormat::MetadataHeader>();

	const size_t chunk_count = 1;
	const OffsetHandle chunk_header_offset = file_builder.ReserveArray<ResourceFileFormat::Chunk>(chunk_count);
	const OffsetHandle metadata_chunk_offset = file_builder.Reserve<TextureMetadata>();
	const OffsetHandle mip_info_offset = file_builder.ReserveArray<MipLevelInfo>(texture.GetMipCount());

	const size_t metadata_heap_size = file_builder.GetSize() - metadata_header_offset;


	const size_t gpu_chunk_data_size = texture.pixel_data.Size();
	OffsetHandle gpu_chunk_heap_offset = file_builder.Reserve(gpu_chunk_data_size, 16u);

	file_builder.Commit();
	{
		auto* header = file_builder.PlaceType<ResourceFileFormat::Header>(header_offset);

		header->Magic = ResourceFileFormat::MagicNumber;
		header->Version = ResourceFileFormat::Version;
		header->BuildNumber = FileFormat::GetTimestamp();
		header->HandlerId = phx::ResourceFileHandlerId<TextureResourceHandler>::value.Value();
		header->MetadataHeapSize = metadata_heap_size;
	}

	auto* metdata_header = file_builder.PlaceType<ResourceFileFormat::MetadataHeader>(metadata_header_offset);
	{
		metdata_header->NumChunks = chunk_count;
		metdata_header->NumStrings = 0;
	}

	{
		auto* texture_metadata			= file_builder.PlaceType<TextureMetadata>(metadata_chunk_offset);
		texture_metadata->width			= texture.width;
		texture_metadata->height		= texture.height;
		texture_metadata->depth			= texture.depth;
		texture_metadata->array_layers	= texture.array_layers;
		texture_metadata->format		= texture.format;
		texture_metadata->mip_levels	= static_cast<uint32_t>(texture.mip_offsets.size());
		metdata_header->MetadataChunk.Set(texture_metadata);

		auto* mip_info = file_builder.PlaceType<MipLevelInfo>(mip_info_offset);
		for (size_t i = 0; i < texture.GetMipCount(); ++i)
		{
			mip_info[i].offset_in_uncompressed = texture.mip_offsets[i];
		}
		texture_metadata->mip_info.Set(mip_info);
	}

	{
		auto* chunk_headers = file_builder.PlaceType<ResourceFileFormat::Chunk>(chunk_header_offset);
		metdata_header->Chunks.Set(chunk_headers);

		chunk_headers[0].Compression = FileFormat::CompressionType::None;
		chunk_headers[0].Offset.Offset = gpu_chunk_heap_offset;
		chunk_headers[0].UncompressedSize = gpu_chunk_data_size;
		chunk_headers[0].CompressedSize = chunk_headers[0].UncompressedSize;
	}

	// -- Gpu Chunk Data ---
	{
		std::byte* gpu_chunk_data = reinterpret_cast<std::byte*>(file_builder.Place(gpu_chunk_heap_offset));
		std::memcpy(
			gpu_chunk_data,
			texture.pixel_data.Data(),
			texture.pixel_data.Size());
	}

	MemoryBuffer file_data_buffer = file_builder.Finalize();
	out.write(reinterpret_cast<const char*>(file_data_buffer.Data()), file_data_buffer.Size());

	return true;
}
