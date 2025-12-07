#include "ResourceFileBuilder.h"

#include <PhxCore/BinaryBuilder.h>
#include <PhxResource/ResourceFileFormat.h>
#include <PhxRenderer/ModelResourceHandler.h>

#include <string>
using namespace phx;

std::unique_ptr<phx::IBlob> ResourceFileBuilder::Build()
{
    std::string filename = std::format("{}{}", m_resource->name, m_resource->ext);
    const size_t stringHeapSize = filename.size() + 1;

    // Build THe pack file
    BinaryBuilder fileBuilder;
    OffsetHandle headerOffset = fileBuilder.Reserve<ResourceFileFormat::Header>();

    // -- metadata heap offsets ---
    OffsetHandle metadataHeaderOffset = fileBuilder.Reserve<ResourceFileFormat::MetadataHeader>();

    OffsetHandle chunkOffsetHandle = fileBuilder.ReserveArray<ResourceFileFormat::Chunk>(m_resource->chunks.size());
    OffsetHandle metadataChunksOffsetHandle = fileBuilder.Reserve(m_resource->metadata_chunk.Size());

    OffsetHandle stringTableOffset = fileBuilder.ReserveArray<FileFormat::StringEntry>(1);
    OffsetHandle stringHeapOffsetHandle = fileBuilder.Reserve(stringHeapSize);

    // Calculate the meta chunk sizes as they are part of the header
    const size_t metadataHeapSize = fileBuilder.GetSize() - metadataHeaderOffset;

    // -- Chunk heap offsets ---
    size_t chunkHeapSize = 0;
    for (auto& chunk : m_resource->chunks)
    {
        chunkHeapSize += chunk.Size();
    }
    OffsetHandle chunkHeapOffset = fileBuilder.Reserve(chunkHeapSize);

    fileBuilder.Commit();

    {
        auto* header = fileBuilder.Place<ResourceFileFormat::Header>(headerOffset);
        header->Magic = ResourceFileFormat::MagicNumber;
        header->Version = ResourceFileFormat::Version;
        header->BuildNumber = FileFormat::GetTimestamp();
        header->HandlerId = phx::ResourceFileHandlerId<renderer::ModelResourceHandler>::value.Value();
        header->MetadataHeapSize = metadataHeapSize;
    }

    {

        auto* chunkOffsetDest = fileBuilder.Place<ResourceFileFormat::Chunk>(chunkOffsetHandle);
        auto* metadataChunksDest = fileBuilder.Place<char>(metadataChunksOffsetHandle);
        auto* stringTableDest = fileBuilder.Place<FileFormat::StringEntry>(stringTableOffset);
        auto* stringDataDest = fileBuilder.Place<char>(stringHeapOffsetHandle);

        auto* chunkHeapDest = fileBuilder.Place<char>(chunkHeapOffset);
        // -- Fill metadata header ---
        {
            auto* metadataHeader = fileBuilder.Place<ResourceFileFormat::MetadataHeader>(metadataHeaderOffset);
            metadataHeader->MetadataChunk.Set(metadataChunksDest);

            std::memcpy(metadataChunksDest, m_resource->metadata_chunk.Data(), m_resource->metadata_chunk.Size());
            metadataChunksDest = metadataChunksDest + m_resource->metadata_chunk.Size();

            metadataHeader->NumChunks = m_resource->chunks.size();
            metadataHeader->Chunks.Set(chunkOffsetDest);

            OffsetHandle chunkHeapCurrentOffset = 0;
            for (size_t i = 0; i < m_resource->chunks.size(); i++)
            {
                auto& srcChunk = m_resource->chunks[i];

                // Construct chunk Header
                ResourceFileFormat::Chunk& destChunk = metadataHeader->Chunks.Get()[i];
                destChunk.Compression = FileFormat::CompressionType::None;
                destChunk.Offset.Offset = chunkHeapOffset + chunkHeapCurrentOffset;
                destChunk.CompressedSize = srcChunk.Size();
                destChunk.UncompressedSize = srcChunk.Size();

                std::memcpy(chunkHeapDest + chunkHeapCurrentOffset, srcChunk.Data(), srcChunk.Size());
                chunkHeapCurrentOffset += srcChunk.Size();
            }

            metadataHeader->NumStrings = 1;
            metadataHeader->StringEntries.Set(stringTableDest);

            char* stringHeapDest = stringDataDest;
            FileFormat::StringEntry& strEntry = metadataHeader->StringEntries.Get()[0];
            strEntry.Hash = StringHash(filename);
            strEntry.Value.Set(stringHeapDest);

            strcpy(stringHeapDest, filename.c_str());

            // TODO
            metadataHeader->NumDependencies = 0;
        }
    }

    return fileBuilder.Finalize();
}
