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
		header->HandlerId = 0;
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

namespace
{
	// Standard DDS Magic and structs for BC7 (DX10)
	const uint32_t DDS_MAGIC = 0x20534444; // "DDS "
	const uint32_t DXGI_FORMAT_BC7_UNORM = 98;
	const uint32_t DXGI_FORMAT_BC7_UNORM_SRGB = 99;

	struct DDS_PIXELFORMAT 
	{
		uint32_t dwSize = 32;
		uint32_t dwFlags = 0x4; // DDPF_FOURCC
		uint32_t dwFourCC = 0x30315844; // "DX10"
		uint32_t dwRGBBitCount = 0;
		uint32_t dwRBitMask = 0;
		uint32_t dwGBitMask = 0;
		uint32_t dwBBitMask = 0;
		uint32_t dwABitMask = 0;
	};

	struct DDS_HEADER 
	{
		uint32_t dwSize = 124;
		uint32_t dwFlags = 0x1007; // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
		uint32_t dwHeight;
		uint32_t dwWidth;
		uint32_t dwPitchOrLinearSize = 0;
		uint32_t dwDepth = 0;
		uint32_t dwMipMapCount = 1;
		uint32_t dwReserved1[11] = { 0 };
		DDS_PIXELFORMAT ddspf;
		uint32_t dwCaps = 0x1000; // DDSCAPS_TEXTURE
		uint32_t dwCaps2 = 0;
		uint32_t dwCaps3 = 0;
		uint32_t dwCaps4 = 0;
		uint32_t dwReserved2 = 0;
	};

	struct DDS_HEADER_DXT10 
	{
		uint32_t dxgiFormat = 98; // DXGI_FORMAT_BC7_UNORM (use 99 for SRGB)
		uint32_t resourceDimension = 3; // D3D10_RESOURCE_DIMENSION_TEXTURE2D
		uint32_t miscFlag = 0;
		uint32_t arraySize = 1;
		uint32_t miscFlags2 = 0;
	};
}
bool phx::renderer::compiler::IntermediateTextureExporter::ExportBC7ToDDS(IntermediateTexture const& texture, std::ostream& out)
{
	if (texture.format != rhi::Format::BC7_UNORM && texture.format != rhi::Format::BC7_UNORM_SRGB)
	{
		PHX_CORE_WARN("IntermediateTextureExporter::ExportBC7ToDDS only supports BC7 formats.");
		return false;
	}

	// 1. Determine Mip Count
	// If offsets is empty, we assume just 1 mip (the main image).
	uint32_t mip_count = std::max(1u, (uint32_t)texture.mip_offsets.size());

	DDS_HEADER header;
	memset(&header, 0, sizeof(header)); // Safety clear
	header.dwSize = 124;

	// 2. Update Flags
	// Standard: CAPS | HEIGHT | WIDTH | PIXELFORMAT | LINEARSIZE
	// Add MIPMAPCOUNT (0x20000) if we have more than 1 mip
	header.dwFlags = 0x00081007 | (mip_count > 1 ? 0x20000 : 0);

	header.dwHeight = texture.height;
	header.dwWidth = texture.width;

	// 3. Calculate Pitch (Mip 0 Size Only)
	// DDS expects this to be the size of the top-level mip, not the whole file.
	uint32_t blocks_w = std::max(1u, (texture.width + 3) / 4);
	uint32_t blocks_h = std::max(1u, (texture.height + 3) / 4);
	header.dwPitchOrLinearSize = blocks_w * blocks_h * 16;

	header.dwDepth = 0;
	header.dwMipMapCount = mip_count;

	// Pixel Format (Standard DX10)
	header.ddspf.dwSize = 32;
	header.ddspf.dwFlags = 0x4; // DDPF_FOURCC
	header.ddspf.dwFourCC = 0x30315844; // "DX10"

	// 4. Update Caps
	// Standard: TEXTURE (0x1000)
	// Add COMPLEX (0x8) and MIPMAP (0x400000) if > 1 mip
	header.dwCaps = 0x1000 | (mip_count > 1 ? 0x400008 : 0);

	// DXT10 Header
	DDS_HEADER_DXT10 header10;
	memset(&header10, 0, sizeof(header10));
	header10.dxgiFormat = texture.format == rhi::Format::BC7_UNORM_SRGB
		? DXGI_FORMAT_BC7_UNORM_SRGB
		: DXGI_FORMAT_BC7_UNORM;
	header10.resourceDimension = 3; // TEXTURE2D
	header10.arraySize = 1;         // Assuming single texture, not array
	header10.miscFlag = 0;

	// Write to stream
	out.write(reinterpret_cast<const char*>(&DDS_MAGIC), sizeof(uint32_t));
	out.write(reinterpret_cast<const char*>(&header), sizeof(DDS_HEADER));
	out.write(reinterpret_cast<const char*>(&header10), sizeof(DDS_HEADER_DXT10));

	// Write the actual pixel data
	// ASSUMPTION: 'pixel_data' contains all mips packed contiguously (Mip0, Mip1, Mip2...)
	// If your vector has gaps/padding, this needs to change to write chunks based on mip_offsets.
	out.write(reinterpret_cast<const char*>(texture.pixel_data.Data()), texture.pixel_data.Size());

	return true;
}