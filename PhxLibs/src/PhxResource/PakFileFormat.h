#pragma once

#include <PhxCore/Base.h>
#include "FileFormatUtils.h"
#include "ResourceFileFormat.h"

namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = FileFormat::MakeMagicNum('P', 'X', 'P', 'K');

        struct Header
        {
            uint32_t Magic;                     // 'PXPK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp

            uint32_t MetadataHeapSize;
            uint8_t _FreeSpace[40];
        };
        CompileTimeAssertSize(Header, 64);

        struct StringEntry;
        struct AssetEntry;
        struct MetadataHeader
        {
            uint32_t NumEntries;                
            FileFormat::RelativePtr<AssetEntry>  AssetEntries;

            uint32_t NumStrings;
            FileFormat::RelativePtr<StringEntry> StringEntries;
        };

        struct AssetEntry
        {
            uint32_t Hash; // Hash of filename for lookup

            // For optmization, we store the metadata chunk right in this metadata heap
            FileFormat::RelativePtr<void*> MetadataChunk;

            // Info about the meteadata chunks.
            uint32_t NumChunks;
            FileFormat::RelativePtr<ResourceFileFormat::ChunkHeader> DataChunkHeaders;
        };

        struct StringEntry
        {
            uint32_t Hash; // Hash of filename for lookup
            FileFormat::RelativePtr<char> Value;
        };
    }
}