#include "MeshResource.h"
#include "MeshFileFormat.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <cstring>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "MeshResource" };
}

void phx::resources::MeshResource::Dispose()
{
    rhi::GpuFree(packed_mesh_buffer);
}

RefCountPtr<MeshResource> phx::resources::CreateMeshResource(MemoryBuffer&& file_bytes)
{
    if (file_bytes.Size() < sizeof(ResourceFileHeader))
    {
        PHX_LOG_ERROR(k_log, "Mesh file too small to contain a header ({0} bytes)", file_bytes.Size());
        return nullptr;
    }

    const auto* header = reinterpret_cast<const ResourceFileHeader*>(file_bytes.Data());
    if (header->magic != kMeshFileMagic)
    {
        PHX_LOG_ERROR(k_log, "Mesh file has the wrong magic number");
        return nullptr;
    }

    if (header->version != kMeshFileVersion)
    {
        PHX_LOG_ERROR(k_log, "Mesh file version mismatch (file={0}, expected={1})", header->version, kMeshFileVersion);
        return nullptr;
    }

    const size_t chunk_table_offset = sizeof(ResourceFileHeader);
    const size_t chunk_table_bytes  = sizeof(ChunkEntry) * static_cast<size_t>(header->chunk_count);
    if (chunk_table_offset + chunk_table_bytes > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Mesh file chunk table is out of bounds");
        return nullptr;
    }

    const auto* chunks = reinterpret_cast<const ChunkEntry*>(file_bytes.Data() + chunk_table_offset);

    const ChunkEntry* cpu_chunk = FindChunk(chunks, header->chunk_count, kMeshChunkType_Cpu);
    const ChunkEntry* gpu_chunk = FindChunk(chunks, header->chunk_count, kMeshChunkType_Gpu);
    if (!cpu_chunk || !gpu_chunk)
    {
        PHX_LOG_ERROR(k_log, "Mesh file is missing its CPU or GPU chunk");
        return nullptr;
    }

    if (static_cast<size_t>(cpu_chunk->offset) + cpu_chunk->size > file_bytes.Size() ||
        static_cast<size_t>(gpu_chunk->offset) + gpu_chunk->size > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Mesh file chunk is out of bounds");
        return nullptr;
    }

    RefCountPtr<MeshResource> res = RefCountPtr<MeshResource>::Create();

    // Upload the GPU chunk verbatim -- it's one contiguous vertex+index blob
    // and DrawInfo's offsets are already relative to its base, so no offset
    // translation happens here.
    res->packed_mesh_buffer = rhi::GpuMalloc(gpu_chunk->size, rhi::GpuMemoryUsage::Upload);
    std::memcpy(res->packed_mesh_buffer.cpu_ptr, file_bytes.Data() + gpu_chunk->offset, gpu_chunk->size);

    const size_t cpu_chunk_offset = cpu_chunk->offset;
    res->cpu_data_buffer = std::move(file_bytes);
    res->cpu_data         = res->cpu_data_buffer.GetView<MeshResource::CpuData>(cpu_chunk_offset);

    res->state = Resource::State::Loaded;
    return res;
}

RefCountPtr<MeshResource> phx::resources::LoadMeshResource(const char* virtual_path)
{
    MemoryBuffer bytes = VFS::ReadFile(virtual_path);
    if (bytes.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Failed to read mesh file '{0}'", virtual_path);
        return nullptr;
    }

    return CreateMeshResource(std::move(bytes));
}
