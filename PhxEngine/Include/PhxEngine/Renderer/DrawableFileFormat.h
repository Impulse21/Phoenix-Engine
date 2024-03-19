#pragma once

#include <stdint.h>

#include <PhxEngine/Resource/ResourceFileFormat.h>

namespace PhxEngine::DrawableFileFormat
{
	constexpr uint16_t CurrentFileVersion = 1u;

	struct Header
	{
		uint32_t ID;
		uint16_t Version;
		GpuRegion UnstructuredGpuData;
		Region<struct CpuMetadataHeader> CpuMetadata;
		Array<Array<char>> Dependencies;

		float BoundingSphere[4];
		float MinPos[3];
		float MaxPos[3];
	};

	struct CpuMetadataHeader
	{
		uint32_t VBOffset;
		uint32_t VBSize;
		uint32_t IBOffset;
		uint32_t IBSize;
		uint32_t MeshletOffset;
		uint32_t MeshletSize;
		uint32_t MeshletVBOffset;
		uint32_t MeshletVBSize;
		uint32_t MeshletBoundsOffset;
		uint32_t MeshletBoundsSize;
		uint8_t IBFormat;
	};
}