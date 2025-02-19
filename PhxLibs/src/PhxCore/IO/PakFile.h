#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/IO/ChunkFile.h"

namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = MakeMagicNum('P', 'X', 'P', 'K');


        /*
                +-----------------------+  <--- Start of File
                |    File Header        |  (Fixed Size)
                |-----------------------|
                |   Asset Entires (N)   | (List of AssetEntries)
                |-----------------------|
                |   String Table (N)    |  (Has, name mappings)
                |-----------------------|
                |   Asset Entry (1)     |  ( Asset ChunkFile )
                |-----------------------|
                |   Asset Entry (1-N)   |
                |-----------------------|
                |   Asset Entry (N)     |
                |-----------------------|
                |   Dependencies Heap   |
                |-----------------------|
                |   String heap         |  (Null terminated string data)
                +-----------------------+
        */
        // Make it 64 bytes so I can expand without changing the Range of the header.
        struct Header
        {
            uint32_t Magic;                     // 'PXPK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp
            uint32_t NumEntries;                // Number of assets in the PAK
            uint32_t NumStrings;
            uint32_t EntriesOffset;
            uint32_t DependenciesHeapSize;       // Depenencies Heap located before the string heaps
            uint32_t StringHeapSize;             // String heap located at the end of the file

            uint8_t _FreeSpace[23];
        };
        CompileTimeAssertSize(Header, 64);

		struct AssetEntry
		{
            uint32_t Hash; // Hash of filename for lookup
            uint64_t Offset;
            uint32_t Size;
            uint32_t NumDependiences;
            uint32_t DependenciesOffset;
		};

        struct StringEntry
        {
            uint32_t Hash; // Hash of filename for lookup
            uint32_t Offset; // Regions offset
        };

    }

    class PakFile
    {
    public:
        // TODO
    };
}