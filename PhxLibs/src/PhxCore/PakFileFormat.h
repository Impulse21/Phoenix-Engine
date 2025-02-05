#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/ChunkFileFormat.h"

namespace phx
{
    struct PakFileHeader
    {
        uint32_t Magic;       // 'PHXPAK'
        uint32_t Version;
        uint64_t BuildNumber;
        uint32_t FileCount;   // Number of files inside
        uint32_t DependencyTableOffset; // Offset to the dependency table
    };

    struct PakDependencyTable
    {
        uint32_t FileNameHash;  // Hash of the file
        uint32_t DependencyCount;
    };
    struct PakEntry
    {
        uint64_t Offset;     // Where in the PAK the file starts
        uint32_t Size;       // Original file size
        uint32_t CompressedSize;
        uint32_t FileNameHash; // Hash of filename for lookup
    };
}