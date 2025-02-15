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
    OffsetHandle assetEntiresOffset = packFileBuilder.ReserveArray<PakFileFormat::AssetEntry>(numEntries);
    OffsetHandle assetsOffset = packFileBuilder.Reserve(m_entiresSize);

    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = MakeMagicNum('P', 'X', 'P', 'K');
        header->Version = PakFileFormat::Version;
        header->BuildNumber = GetTimestamp();
        header->NumEntires = numEntries;
    }

    {
        auto* entriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntiresOffset);
        auto* entreesDataDest = packFileBuilder.Place<char>(assetsOffset);

        OffsetHandle dataOffset = assetsOffset;
        auto entriesItr = m_entries.begin();

        for (size_t i = 0; i < numEntries; i++)
        {
            PakFileFormat::AssetEntry& entry = *(entriesDest + i);
            entry.Offset = dataOffset;
            entry.Size = entriesItr->second->Size();
            entry.FileNameHash = StringHash(entriesItr->first);
            entry.CompressedSize = entry.Size;
            
            char* dataDest = (entreesDataDest + i);
            std::memcpy(dataDest, entriesItr->second->Data(), entry.Size);

            dataOffset += entry.Size;
            std::advance(entriesItr, 1);
        }
    }

    return packFileBuilder.Finialize();
}
