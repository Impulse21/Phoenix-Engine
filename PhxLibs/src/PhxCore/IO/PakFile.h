#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/IO/ChunkFile.h"

namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = MakeMagicNum('P', 'X', 'P', 'K');
        struct DEFINE_ALIGNED(Header, 64)
        {
            uint32_t Magic;                     // 'PHXPAK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp
            uint32_t FileCount;                 // Number of files inside
            uint32_t DependencyTableOffset;     // Offset to the dependency table
            uint32_t NumEntires;                // Entires follow after header
        };

        static_assert(sizeof(Header) == 64);

        struct AssetEntry
        {
            uint64_t Offset;     // Where in the PAK the file starts
            uint32_t Size;       // Original file size
            uint32_t CompressedSize;
            uint32_t FileNameHash; // Hash of filename for lookup
        };
    }

    class PakFile
    {
    public:
        // TODO
    };
}