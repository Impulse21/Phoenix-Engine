#include "PakFileBuilder.h"

#include <PhxCore/StringHash.h>
#include <PhxCore/VFS.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxCore/IO/PakFile.h>

using namespace phx;

std::unique_ptr<IBlob> phx::PakFileBuilder::Build()
{
    // Build THe pack file
    BinaryBuilder packFileBuilder;
    OffsetHandle headerOffset = packFileBuilder.Reserve<PakFileFormat::Header>();

    const size_t numEntries = m_entries.size();
    OffsetHandle assetsOffset = packFileBuilder.Reserve(m_entiresSize);
    OffsetHandle assetEntriesOffset = packFileBuilder.ReserveArray<PakFileFormat::AssetEntry>(numEntries);
    // TODO: Build string table

    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = PakFileFormat::MagicNumber;
        header->Version = PakFileFormat::Version;
        header->BuildNumber = GetTimestamp();
        header->AssetCount = numEntries;
        header->AssetEntires.Data.Offset = assetEntriesOffset;
        header->AssetEntires.Size = sizeof(PakFileFormat::AssetEntry) * numEntries;
    }

    {
        auto* entriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntriesOffset);
        auto* entreesDataDest = packFileBuilder.Place<char>(assetsOffset);

        OffsetHandle dataOffset = assetsOffset;
        auto entriesItr = m_entries.begin();

        for (size_t i = 0; i < numEntries; i++)
        {
            PakFileFormat::AssetEntry& entry = *(entriesDest + i);
            entry.AssetHeader.Data.Offset = dataOffset;
            entry.AssetHeader.Size = sizeof(ChunkFileFormat::Header);
            entry.FileNameHash = StringHash(entriesItr->first);
            
            const size_t chunkFileSize = entriesItr->second->Size();
            char* dataDest = (entreesDataDest + i);
            std::memcpy(dataDest, entriesItr->second->Data(), chunkFileSize);

            dataOffset += chunkFileSize;
            std::advance(entriesItr, 1);
        }
    }

    return packFileBuilder.Finialize();
}
