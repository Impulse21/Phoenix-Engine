#include "PakFileBuilder.h"

#include <PhxCore/StringHash.h>
#include <PhxCore/VFS.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxResource/PakFileFormat.h>
#include <PhxResource/FileFormatUtils.h>

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
    OffsetHandle stringHeapOffsetHandle = packFileBuilder.Reserve(m_stringHeapSize);

    //
    // TODO: Build string table

    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = PakFileFormat::MagicNumber;
        header->Version = PakFileFormat::Version;
        header->BuildNumber = FileFormat::GetTimestamp();
        header->NumEntries = numEntries;
        header->EntriesOffset = static_cast<uint32_t>(assetEntriesOffset);
        header->NumStrings = numEntries;
        header->DependenciesHeapSize = 0;
        header->StringHeapSize = m_stringHeapSize;
    }

    {
        auto* entriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntriesOffset);
        auto* entreesDataDest = packFileBuilder.Place<char>(assetsOffsetHandle);

        auto* stringTableDest = packFileBuilder.Place<PakFileFormat::StringEntry>(stringTableOffset);
        auto* stringDataDest = packFileBuilder.Place<char>(stringHeapOffsetHandle);

        OffsetHandle dataOffset = assetsOffsetHandle;
        OffsetHandle strDataOffset = 0;

        auto entriesItr = m_entries.begin();

        for (size_t i = 0; i < numEntries; i++)
        {
            uint32_t hash = entriesItr->first;
            std::pair<std::string, IBlob*>& entry = entriesItr->second;

            PakFileFormat::AssetEntry& assetEntry = *(entriesDest + i);
            assetEntry.Offset = dataOffset;
            assetEntry.Size = entry.second->Size();
            assetEntry.Hash = hash;
            assetEntry.NumDependiences = 0;
            
            char* dataDest = (entreesDataDest + i);
            std::memcpy(dataDest, entry.second->Data(), assetEntry.Size);

            PakFileFormat::StringEntry& strEntry = *(stringTableDest + i);
            strEntry.Hash = assetEntry.Hash;
            strEntry.Offset = strDataOffset;


            char* strDataDest = (stringDataDest + strDataOffset);
            strcpy(strDataDest, entry.first.c_str());

            dataOffset += assetEntry.Size;
            strDataOffset += entry.first.size() + 1;
            std::advance(entriesItr, 1);
        }
    }

    return packFileBuilder.Finialize();
}
