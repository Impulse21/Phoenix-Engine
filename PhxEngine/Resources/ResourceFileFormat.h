#pragma once

#include <cstdint>

namespace phx::resources
{
    // Generic on-disk container shared by every cooked resource type.
    // Deliberately much simpler than a full asset-pipeline format (no
    // compression, no dependency graph): a fixed header + a chunk table,
    // where each chunk is tagged by a FourCC `type` and the resource's own
    // loader knows which tags to expect and how to interpret them. The
    // container itself stays agnostic -- `magic` says which resource kind a
    // file is, chunk tags say what's inside each chunk.
    //
    // Layout: ResourceFileHeader, then `chunk_count` ChunkEntry records,
    // then each chunk's payload (16-byte aligned), then the string table.

    constexpr u32 MakeFourCC(char a, char b, char c, char d)
    {
        return (static_cast<u32>(a) << 0) | (static_cast<u32>(b) << 8) |
               (static_cast<u32>(c) << 16) | (static_cast<u32>(d) << 24);
    }

    struct ResourceFileHeader
    {
        u32 magic;                // per resource kind, e.g. kMeshFileMagic
        u16 version;              // bumped per-format on any layout change
        u16 chunk_count;
        u32 source_path_offset;   // into the trailing string table; NUL-terminated; 0 = absent
        u64 source_content_hash;  // hash of the source asset's bytes; staleness check that survives touch/rename
        u32 strings_offset;
        u32 strings_size;
    };
    static_assert(sizeof(ResourceFileHeader) == 32);

    struct ChunkEntry
    {
        u32 type;    // FourCC tag -- loader dispatches on this, not on array index
        u32 offset;  // file-relative
        u32 size;
    };
    static_assert(sizeof(ChunkEntry) == 12);

    // Returns nullptr if no chunk with `type` is present.
    inline const ChunkEntry* FindChunk(const ChunkEntry* chunks, u16 chunk_count, u32 type)
    {
        for (u16 i = 0; i < chunk_count; ++i)
        {
            if (chunks[i].type == type)
                return &chunks[i];
        }
        return nullptr;
    }
}
