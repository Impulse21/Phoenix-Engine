#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/IO/ChunkFile.h"

namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = MakeMagicNum('P', 'X', 'P', 'K');


        // Make it 64 bytes so I can expand without changing the Range of the header.
        struct DEFINE_ALIGNED(Header, 64)
        {
            uint32_t Magic;                     // 'PHXPAK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp
            uint32_t NumEntries;                // Number of assets in the PAK
            uint64_t EntriesOffset;
            uint32_t NumStrings;
            uint32_t StringTableOffset;
            uint32_t StringDataOffset;
            uint64_t StringDataSize;
        };

        static_assert(sizeof(Header) == 64, "PAK Header must be 64 bytess for alignment");

		struct AssetEntry
		{
            uint32_t Hash; // Hash of filename for lookup
            uint64_t Offset;
            uint32_t Size;
            uint32_t NumDependiences;
            uint32_t Dependencies[];
		};

        struct StringEntry
        {
            uint32_t Hash; // Hash of filename for lookup
            uint32_t Offset;
        };

        /*
                +-----------------------+  <--- Start of File
                |    File Header        |  (Fixed Size)
                |-----------------------|
                |   Asset Index (N)     | (List of AssetEntries)
                |-----------------------|
                |   String Table (N)    |  (Has, name mappings)
                |-----------------------|
                |   Asset Entry (1)     |  ( Asset ChunkFile )
                |-----------------------|
                |   Asset Entry (1-N)   |
                |-----------------------|
                |   Asset Entry (N)     |
                |-----------------------|
                |   String Data         |  (Null terminated string data)
                +-----------------------+
        */
    }

    class PakFile
    {
    public:
        // TODO
    };
}