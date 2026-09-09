#include "MaterialResource.h"
#include "MaterialFileFormat.h"
#include "CookedPathBuilder.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/VFS/VFS.h>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "MaterialResource" };
}

RefCountPtr<MaterialResource> phx::resources::CreateMaterialResource(MemoryBuffer&& file_bytes)
{
    if (file_bytes.Size() < sizeof(ResourceFileHeader))
    {
        PHX_LOG_ERROR(k_log, "Material file too small to contain a header ({0} bytes)", file_bytes.Size());
        return nullptr;
    }

    const auto* header = reinterpret_cast<const ResourceFileHeader*>(file_bytes.Data());
    if (header->magic != kMaterialFileMagic)
    {
        PHX_LOG_ERROR(k_log, "Material file has the wrong magic number");
        return nullptr;
    }

    if (header->version != kMaterialFileVersion)
    {
        PHX_LOG_ERROR(k_log, "Material file version mismatch (file={0}, expected={1})", header->version, kMaterialFileVersion);
        return nullptr;
    }

    const size_t chunk_table_offset = sizeof(ResourceFileHeader);
    const size_t chunk_table_bytes  = sizeof(ChunkEntry) * static_cast<size_t>(header->chunk_count);
    if (chunk_table_offset + chunk_table_bytes > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Material file chunk table is out of bounds");
        return nullptr;
    }

    const auto* chunks = reinterpret_cast<const ChunkEntry*>(file_bytes.Data() + chunk_table_offset);
    const ChunkEntry* cpu_chunk = FindChunk(chunks, header->chunk_count, kMaterialChunkType_Cpu);
    if (!cpu_chunk)
    {
        PHX_LOG_ERROR(k_log, "Material file is missing its CPU chunk");
        return nullptr;
    }

    if (static_cast<size_t>(cpu_chunk->offset) + cpu_chunk->size > file_bytes.Size())
    {
        PHX_LOG_ERROR(k_log, "Material file chunk is out of bounds");
        return nullptr;
    }

    const auto* cpu_data = reinterpret_cast<const MaterialResource::CpuData*>(file_bytes.Data() + cpu_chunk->offset);

    RefCountPtr<MaterialResource> res = RefCountPtr<MaterialResource>::Create();
    res->gpu_data = cpu_data->data;

    for (u32 slot = 0; slot < Slot_Count; ++slot)
    {
        const u32 path_offset = cpu_data->texture_path_offsets[slot];
        if (path_offset == 0)
            continue;

        const char* path = GetString(header, file_bytes.Data(), path_offset);
        RefCountPtr<TextureResource> tex = LoadTextureResource(path);
        if (!tex)
        {
            PHX_LOG_ERROR(k_log, "Material: failed to load texture '{0}' for slot {1}", path, slot);
            continue;
        }

        res->textures[slot] = tex;

        switch (slot)
        {
            case Slot_BaseColor:         res->gpu_data.base_colour_texture        = tex->bindless_index; break;
            case Slot_Normal:            res->gpu_data.normal_texture             = tex->bindless_index; break;
            case Slot_MetallicRoughness: res->gpu_data.metallic_roughness_texture = tex->bindless_index; break;
            case Slot_Occlusion:         res->gpu_data.occlusion_texture          = tex->bindless_index; break;
            case Slot_Emissive:          res->gpu_data.emissive_texture           = tex->bindless_index; break;
            default: break;
        }
    }

    const size_t cpu_chunk_offset = cpu_chunk->offset;
    res->cpu_data_buffer = std::move(file_bytes);
    res->cpu_data         = res->cpu_data_buffer.GetView<MaterialResource::CpuData>(cpu_chunk_offset);

    res->state = Resource::State::Loaded;
    return res;
}

RefCountPtr<MaterialResource> phx::resources::LoadMaterialResource(const char* virtual_path)
{
    MemoryBuffer bytes = VFS::ReadFile(virtual_path);
    if (bytes.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Failed to read material file '{0}'", virtual_path);
        return nullptr;
    }

    return CreateMaterialResource(std::move(bytes));
}

RefCountPtr<MaterialResource> phx::resources::LoadMaterialForDrawInfo(const MeshResource& mesh, const MeshResource::CpuData::DrawInfo& draw_info)
{
    if (draw_info.material_name_offset == 0)
        return nullptr;

    const auto* mesh_header = reinterpret_cast<const ResourceFileHeader*>(mesh.cpu_data_buffer.Data());
    const char* source_path   = GetString(mesh_header, mesh.cpu_data_buffer.Data(), mesh_header->source_path_offset);
    const char* material_name = GetString(mesh_header, mesh.cpu_data_buffer.Data(), draw_info.material_name_offset);

    if (*source_path == '\0' || *material_name == '\0')
        return nullptr;

    return LoadMaterialResource(CookedMaterialPath(source_path, material_name).c_str());
}
