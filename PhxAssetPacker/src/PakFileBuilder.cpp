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
	OffsetHandle chunkOffsetHandle = packFileBuilder.Reserve<ResourceFileFormat::Chunk>(m_numChunks);
    OffsetHandle stringTableOffset = packFileBuilder.ReserveArray<PakFileFormat::StringEntry>(numEntries);

	OffsetHandle metadataChunksOffsetHandle = packFileBuilder.Reserve(m_metadataChunksSize);
	OffsetHandle stringHeapOffsetHandle = packFileBuilder.Reserve(m_stringHeapSize);

    // Calculate the meta chunk sizes as they are part of the header
    const size_t metadataHeapSize = packFileBuilder.GetSize() - metadataHeaderOffset;

    // -- Data Offsets ---
    OffsetHandle chunkHeapOffset = packFileBuilder.Reserve(m_chunkHeapSize);

    //
    // TODO: Build string table
    
    packFileBuilder.Commit();

    {
        auto* header = packFileBuilder.Place<PakFileFormat::Header>(headerOffset);
        header->Magic = PakFileFormat::MagicNumber;
        header->Version = PakFileFormat::Version;
        header->BuildNumber = FileFormat::GetTimestamp();
        header->MetadataHeapSize = metadataHeapSize;
    }

    {
        auto* assetEntriesDest = packFileBuilder.Place<PakFileFormat::AssetEntry>(assetEntriesOffset);
        auto* chunkHeadersDest = packFileBuilder.Place<ResourceFileFormat::Chunk>(chunkOffsetHandle);
        auto* metadataChunksDest = packFileBuilder.Place<char>(metadataChunksOffsetHandle);
        auto* stringTableDest = packFileBuilder.Place<PakFileFormat::StringEntry>(stringTableOffset);
        auto* stringDataDest = packFileBuilder.Place<char>(stringHeapOffsetHandle);

        auto* chunkHeapDest = packFileBuilder.Place<char>(chunkHeapOffset);

        // -- Fill metadata header ---
        {
            auto* metadataHeader = packFileBuilder.Place<PakFileFormat::MetadataHeader>(metadataHeaderOffset);
            metadataHeader->NumEntries = numEntries;
            metadataHeader->AssetEntries.Set(assetEntriesDest);

            metadataHeader->NumStrings = numEntries;
            metadataHeader->StringEntries.Set(stringTableDest);

            // TODO
			metadataHeader->NumDependencies= 0;
        }

        OffsetHandle strDataOffset = 0;
        OffsetHandle chunkDestByteOffset = 0;
        auto entriesItr = m_entries.begin();
        for (size_t i = 0; i < m_entries.size(); i++)
        {
            auto& [filename, compiledResource] = *entriesItr;

            PakFileFormat::AssetEntry& assetEntry = *(assetEntriesDest + i);
            assetEntry.Hash = phx::StringHash(filename);
            assetEntry.HandlerId = 0; // TODO
            
			std::memcpy(metadataChunksDest, compiledResource.MetadataChunk->Data(), compiledResource.MetadataChunk->Size());
			metadataChunksDest = metadataChunksDest + compiledResource.MetadataChunk->Size();

            for (auto& [id, chunk] : compiledResource->Chunks)
			{
				// Construct chunk Header
				*chunkHeadersDest = {
					.Compression = FileFormat::CompressionType::None,
					.Offset = static_cast<uint32_t>(chunkHeapOffset + chunkDestByteOffset),
					.CompressedSize = static_cast<uint32_t>(chunk->Size()),
					.UncompressedSize = static_cast<uint32_t>(chunk->Size()),
				};

				std::memcpy(chunkHeapDest + chunkDestByteOffset, chunk->Data(), chunk->Size());
				chunkDestByteOffset += chunk->Size();
				chunkHeadersDest += 1;
				assetEntry.NumChunks += 1;
            }

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
