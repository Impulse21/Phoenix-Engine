#pragma once

#include <stdint.h>
#include "Core/phxRelativePtr.h"

#define MAKE_ID(a, b, c, d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))

namespace phx
{
	namespace arc
	{

        constexpr uint16_t CURRENT_PARC_FILE_VERSION = 1u;
        constexpr uint32_t Id = MAKE_ID('P', 'A', 'R', 'C');

        //
        // Supported compression formats.  See Region.
        //
        enum class Compression : uint16_t
        {
            None = 0,
            GDeflate = 1,
            Zlib = 2,
        };

        template<typename T>
        struct Region
        {
            Compression Compression;
            RelativePtr<T> Data; // (on disk this is compressed, in memory it is uncompressed)
            uint32_t CompressedSize;
            uint32_t UncompressedSize;
        };

        using GpuRegion = Region<void>;

        struct Header
        {
            uint32_t Id;       // "parc"
            uint16_t Version; // CURRENT_PARC_FILE_VERSION

            // The unstructured GPU data is read entirely into a D3D12 buffer resource.
            GpuRegion UnstructuredGpuData;
            Region<struct CpuMetadataHeader> CpuMetadata;
            Region<struct CpuDataHeader> CpuData;

            float BoundingSphere[4];
            float MinPos[3];
            float MaxPos[3];
        };
	}
}