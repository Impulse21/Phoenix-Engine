#include "TextureResource.h"
#include "TextureFileFormat.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <vector>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "TextureResource" };
}

void phx::resources::TextureResource::Dispose()
{
    rhi::DestroyTexture(texture);
}

RefCountPtr<TextureResource> phx::resources::CreateTextureResource(MemoryBuffer&& file_bytes)
{
    if (file_bytes.Size() < sizeof(ResourceFileHeader))
    {
        PHX_LOG_ERROR(k_log, "Texture file too small to contain a header ({0} bytes)", file_bytes.Size());
        return nullptr;
    }

    const auto* header = reinterpret_cast<const ResourceFileHeader*>(file_bytes.Data());
    if (header->magic != kTextureFileMagic)
    {
        PHX_LOG_ERROR(k_log, "Texture file has the wrong magic number");
        return nullptr;
    }

    if (header->version != kTextureFileVersion)
    {
        PHX_LOG_ERROR(k_log, "Texture file version mismatch (file={0}, expected={1})", header->version, kTextureFileVersion);
        return nullptr;
    }

    const size_t chunk_table_offset = sizeof(ResourceFileHeader);
    const size_t chunk_table_bytes  = sizeof(ChunkEntry) * static_cast<size_t>(header->chunk_count);
    if (chunk_table_offset + chunk_table_bytes > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Texture file chunk table is out of bounds");
        return nullptr;
    }

    const auto* chunks = reinterpret_cast<const ChunkEntry*>(file_bytes.Data() + chunk_table_offset);

    const ChunkEntry* cpu_chunk = FindChunk(chunks, header->chunk_count, kTextureChunkType_Cpu);
    const ChunkEntry* gpu_chunk = FindChunk(chunks, header->chunk_count, kTextureChunkType_Gpu);
    if (!cpu_chunk || !gpu_chunk)
    {
        PHX_LOG_ERROR(k_log, "Texture file is missing its CPU or GPU chunk");
        return nullptr;
    }

    if (static_cast<size_t>(cpu_chunk->offset) + cpu_chunk->size > file_bytes.Size() ||
        static_cast<size_t>(gpu_chunk->offset) + gpu_chunk->size > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Texture file chunk is out of bounds");
        return nullptr;
    }

    const auto* cpu_data = reinterpret_cast<const TextureResource::CpuData*>(file_bytes.Data() + cpu_chunk->offset);
    if (cpu_data->mip_count == 0)
    {
        PHX_LOG_ERROR(k_log, "Texture file has zero mips");
        return nullptr;
    }

    const TextureResource::CpuData::MipInfo* mips = cpu_data->mips.Get();

    std::vector<rhi::TextureUploadRegion> regions;
    regions.reserve(cpu_data->mip_count);
    for (u32 i = 0; i < cpu_data->mip_count; ++i)
    {
        regions.push_back(rhi::TextureUploadRegion{
            .data        = file_bytes.Data() + gpu_chunk->offset + mips[i].byte_offset,
            .size        = mips[i].byte_size,
            .mip_level   = i,
            .array_slice = 0,
            .width       = mips[i].width,
            .height      = mips[i].height,
            .depth       = 1,
        });
    }

    const rhi::TextureDescriptor desc{
        .format     = static_cast<rhi::Format>(cpu_data->format),
        .width      = cpu_data->width,
        .height     = cpu_data->height,
        .mip_levels = static_cast<u16>(cpu_data->mip_count),
    };

    RefCountPtr<TextureResource> res = RefCountPtr<TextureResource>::Create();
    res->texture = rhi::CreateTextureWithData(desc, Span<const rhi::TextureUploadRegion>(regions.data(), regions.size()));
    if (!res->texture.IsValid())
    {
        PHX_LOG_ERROR(k_log, "Failed to create/upload GPU texture");
        return nullptr;
    }

    res->bindless_index = rhi::GetShaderResourceIndex(res->texture);

    const size_t cpu_chunk_offset = cpu_chunk->offset;
    res->cpu_data_buffer = std::move(file_bytes);
    res->cpu_data         = res->cpu_data_buffer.GetView<TextureResource::CpuData>(cpu_chunk_offset);

    res->state = Resource::State::Loaded;
    return res;
}

RefCountPtr<TextureResource> phx::resources::LoadTextureResource(const char* virtual_path)
{
    MemoryBuffer bytes = VFS::ReadFile(virtual_path);
    if (bytes.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Failed to read texture file '{0}'", virtual_path);
        return nullptr;
    }

    return CreateTextureResource(std::move(bytes));
}
