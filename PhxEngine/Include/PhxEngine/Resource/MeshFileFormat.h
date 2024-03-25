#pragma once

#include <PhxEngine/Resource/ResourceFileFormat.h>
#include <PhxEngine/Core/Primitives.h>

namespace PhxEngine
{
	namespace MeshFileFormat
	{
		constexpr uint32_t ID = MAKE_ID('P', 'M', 'S', 'H');
		constexpr uint16_t CurrentVersion = 1;

		struct Header
		{
			uint32_t ID;
			uint16_t Version;

			GpuRegion GpuData;
			Region<struct MeshMetadataHeader> MeshMetadataHeader;
			DependenciesRegion DependenciesRegion;
			StringHashTableHeader StringHashTableRegion;

			float BoundingSphere[4];
			float MinPos[3];
			float MaxPos[3];
		};

		struct MeshMetadataHeader
		{
			uint32_t Name;
			uint32_t GeometryCount;
			Array<struct Geometry> Geometry;

			// Offsets
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
}