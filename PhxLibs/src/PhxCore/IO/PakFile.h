#pragma once

#include "PhxCore/Base.h"
#include "PhxCore/IO/ChunkFile.h"

namespace phx
{
    namespace PakFileFormat
    {
        constexpr uint32_t Version = 1;
        constexpr uint32_t MagicNumber = MakeMagicNum('P', 'X', 'P', 'K');

        template<typename T>
        union Ptr
        {
            uint64_t Offset;
            T* Ptr;
        };

        template<typename T>
        struct PakRegion
        {
            Ptr<T> Data; // (on disk this is compressed, in memory it is uncompressed)
            uint64_t Size;
        };

		struct AssetEntry
		{
			PakRegion<ChunkFileFormat::Header> AssetHeader;
			uint32_t FileNameHash; // Hash of filename for lookup
		};

        struct StringTable
        {
            Ptr<char> StringValues[];
        };

        /*
                +-----------------------+  <--- Start of File
                |    File Header        |  (Fixed Size)
                |-----------------------|
                |   Chunk Table (N)     |  
                |-----------------------|
                |   Asset Entry (1)     |  ( Asset ChunkFile )
                |-----------------------|
                |   Asset Entry (1-N)   |
                |-----------------------|
                |   Asset Entry (N)     |
                |-----------------------|
                |   Asset Index         | (List of AssetEntries)
                +-----------------------+
        */

        // Make it 64 bytes so I can expand without changing the Range of the header.
        struct DEFINE_ALIGNED(Header, 64)
        {
            uint32_t Magic;                     // 'PHXPAK'
            uint32_t Version;
            uint64_t BuildNumber;               // Build Number is a timestamp
            uint32_t AssetCount;

            PakRegion<AssetEntry> AssetEntires;
            PakRegion<StringTable> StringTables;
        };

        static_assert(sizeof(Header) == 64);   
    }

    class PakFile
    {
    public:
        // TODO
    };
}