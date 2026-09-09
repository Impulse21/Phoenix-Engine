#pragma once

#include <cstdint>
#include <cstddef>

namespace phx::resources
{
    constexpr u32 MakeTag(char a, char b, char c, char d)
    {
        return (static_cast<u32>(a) << 0) | (static_cast<u32>(b) << 8) |
               (static_cast<u32>(c) << 16) | (static_cast<u32>(d) << 24);
    }

    struct ResourceFileHeader
    {
        u32 magic;
        u16 version;
        u16 chunk_count;
        u32 source_path_offset;
        u64 source_content_hash;
        u32 strings_offset;
        u32 strings_size;
    };
    static_assert(sizeof(ResourceFileHeader) == 32);

    struct ChunkEntry
    {
        u32 type;
        u32 offset;
        u32 size;
    };
    static_assert(sizeof(ChunkEntry) == 12);

    inline const ChunkEntry* FindChunk(const ChunkEntry* chunks, u16 chunk_count, u32 type)
    {
        for (u16 i = 0; i < chunk_count; ++i)
        {
            if (chunks[i].type == type)
                return &chunks[i];
        }
        return nullptr;
    }

    inline u64 HashBytes(const std::byte* data, size_t size)
    {
        u64 hash = 14695981039346656037ull;
        for (size_t i = 0; i < size; ++i)
        {
            hash ^= static_cast<u64>(data[i]);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    inline u32 AlignUp(u32 value, u32 alignment)
    {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    inline const char* GetString(const ResourceFileHeader* header, const std::byte* file_base, u32 offset)
    {
        if (offset == 0 || offset >= header->strings_size)
            return "";

        return reinterpret_cast<const char*>(file_base + header->strings_offset + offset);
    }
}
