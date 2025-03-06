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

    // -- metadata heap offsets ---
    OffsetHandle metadataHeaderOffset = packFileBuilder.Reserve<PakFileFormat::MetadataHeader>();

    const size_t numEntries = m_entries.size();
    OffsetHandle assetEntriesOffset = packFileBuilder.ReserveArray<PakFileFormat::AssetEntry>(numEntries);
    OffsetHandle stringTableOffset = packFileBuilder.ReserveArray<PakFileFormat::StringEntry>(numEntries);
    OffsetHandle stringHeapOffsetHandle = packFileBuilder.Reserve(m_stringHeapSize);
    OffsetHandle chunkHeaderOffsetHandle = packFileBuilder.Reserve<ResourceFileFormat::ChunkHeader>(m_numChunkHeaders);
    OffsetHandle metadataChunksOffsetHandle = packFileBuilder.Reserve(m_metadataChunksSize);

    // Calculate the meta chunk sizes as they are part of the header
    const size_t metadataHeapSize = packFileBuilder.GetSize() - metadataHeaderOffset;

    // -- Data Offsets ---
    OffsetHandle assetChunkOffsetHandle = packFileBuilder.Reserve(m_chunkSize);

    //
    // TODO: Build string table
    
    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = PakFileFormat::MagicNumber;
        header->Version = PakFileFormat::Version;
        header->BuildNumber = FileFormat::GetTimestamp();
        header->MetadataHeapSize = static_cast<uint32_t>(metadataHeapSize);
    }

    {
        auto* assetEntriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntriesOffset);
        auto* chunkHeadersDest = packFileBuilder.Place<ResourceFileFormat::ChunkHeader>(chunkHeaderOffsetHandle);
        auto* metadataChunksDest = packFileBuilder.Place<char>(metadataChunksOffsetHandle);
        auto* stringTableDest = packFileBuilder.Place<PakFileFormat::StringEntry>(stringTableOffset);
        auto* stringDataDest = packFileBuilder.Place<char>(stringHeapOffsetHandle);

        auto* assetChunkDest = packFileBuilder.Place<char>(assetChunkOffsetHandle);

        // -- Fill metadata header ---
        {
            auto* metadataHeader = packFileBuilder.Place<PakFileFormat::MetadataHeader>(metadataHeaderOffset);
            metadataHeader->NumEntries = numEntries;
            metadataHeader->AssetEntries.Set(assetEntriesDest);
            metadataHeader->NumStrings = numEntries;
            metadataHeader->StringEntries.Set(stringTableDest);
        }

        OffsetHandle strDataOffset = 0;
        OffsetHandle chunkDestByteOffset = 0;
        auto entriesItr = m_entries.begin();
        for (size_t i = 0; i < m_entries.size(); i++)
        {
            auto& [filename, compiledResource] = *entriesItr;

            PakFileFormat::AssetEntry& assetEntry = *(assetEntriesDest + i);
            assetEntry.Hash = phx::StringHash(filename);

            for (auto& [id, chunk] : compiledResource->Chunks)
            {
                if (id == ResourceFileFormat::ChunkId_Metadata)
                {
                    std::memcpy(metadataChunksDest, chunk->Data(), chunk->Size());
                    metadataChunksDest = metadataChunksDest + chunk->Size();
                }
                else
                {
                    // Construct chunk Header
                    *chunkHeadersDest = {
                    	.ChunkID = id,
                        .Compression = FileFormat::CompressionType::None,
						.Offset = static_cast<uint32_t>(assetChunkOffsetHandle + chunkDestByteOffset),
						.CompressedSize = static_cast<uint32_t>(chunk->Size()),
                        .UncompressedSize = static_cast<uint32_t>(chunk->Size()),
                    };

                    std::memcpy(assetChunkDest + chunkDestByteOffset, chunk->Data(), chunk->Size());
                    chunkDestByteOffset += chunk->Size();
                    chunkHeadersDest += 1;
                    assetEntry.NumChunks += 1;
                }
            }
            //  assetEntry.DataChunkHeaders;
            // assetEntry.MetadataChunk;

            char* strDataDest = (stringDataDest + strDataOffset);
            strcpy(strDataDest, filename.c_str());

            PakFileFormat::StringEntry& strEntry = *(stringTableDest + i);
            strEntry.Hash = assetEntry.Hash;
            strEntry.Value.Set(strDataDest);

            strDataOffset += filename.size() + 1;
            std::advance(entriesItr, 1);
        }
    }

    return packFileBuilder.Finialize();
}
