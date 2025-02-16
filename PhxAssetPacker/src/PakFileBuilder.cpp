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
    OffsetHandle assetEntriesOffset = packFileBuilder.ReserveArray<PakFileFormat::AssetEntry>(numEntries);
    OffsetHandle stringTableOffset = packFileBuilder.ReserveArray<PakFileFormat::StringEntry>(numEntries);
    OffsetHandle assetsOffsetHandle = packFileBuilder.Reserve(m_entiresSize);
    OffsetHandle stringDataOffsetHandle = packFileBuilder.Reserve(m_stringDataSize);

    //
    // TODO: Build string table

    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = PakFileFormat::MagicNumber;
        header->Version = PakFileFormat::Version;
        header->BuildNumber = GetTimestamp();
        header->NumEntries = numEntries;
        header->EntriesOffset = static_cast<uint32_t>(assetEntriesOffset);
        header->NumStrings = numEntries;
        header->StringTableOffset = static_cast<uint32_t>(stringTableOffset);
        header->StringDataOffset = stringDataOffsetHandle;
        header->StringDataSize = m_stringDataSize;
    }

    {
        auto* entriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntriesOffset);
        auto* entreesDataDest = packFileBuilder.Place<char>(assetsOffsetHandle);

        auto* stringTableDest = packFileBuilder.Place<PakFileFormat::StringEntry>(stringTableOffset);
        auto* stringDataDest = packFileBuilder.Place<char>(stringDataOffsetHandle);

        OffsetHandle dataOffset = assetsOffsetHandle;
        OffsetHandle strDataOffset = stringDataOffsetHandle;

        auto entriesItr = m_entries.begin();

        for (size_t i = 0; i < numEntries; i++)
        {
            PakFileFormat::AssetEntry& entry = *(entriesDest + i);
            entry.Offset = dataOffset;
            entry.Size = entriesItr->second->Size();
            entry.Hash = StringHash(entriesItr->first);
            entry.NumDependiences = 0;
            
            char* dataDest = (entreesDataDest + i);
            std::memcpy(dataDest, entriesItr->second->Data(), entry.Size);

            PakFileFormat::StringEntry& strEntry = *(stringTableDest + i);
            strEntry.Hash = entry.Hash;
            strEntry.Offset = strDataOffset;


            char* strDataDest = (stringDataDest + (strDataOffset - stringDataOffsetHandle));
            strcpy(strDataDest, entriesItr->first.c_str());

            dataOffset += entry.Size;
            strDataOffset += entriesItr->first.size() + 1;
            std::advance(entriesItr, 1);
        }
    }

    return packFileBuilder.Finialize();
}
