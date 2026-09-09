#include "MaterialCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Resources/Intermediate/IntermediateModel.h>
#include <PhxEngine/Resources/MaterialFileFormat.h>
#include <PhxEngine/Resources/CookedPathBuilder.h>
#include <PhxEngine/VFS/VFS.h>

#include <cstring>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "MaterialCompiler" };

    std::string ResolveTexturePath(const IntermediateModel& model, u32 texture_index, const std::string& source_path)
    {
        if (texture_index == IntermediateMaterial::kInvalidTextureIndex || texture_index >= model.textures.size())
            return {};

        const IntermediateTexture& tex = model.textures[texture_index];
        if (!tex.IsValid())
            return {};

        return CookedTexturePath(source_path, tex.name);
    }
}

CompiledMaterial phx::resources::CompileMaterial(const IntermediateMaterial& material, const IntermediateModel& model, const std::string& source_path)
{
    if (material.archetype != "standard_pbr")
    {
        PHX_LOG_ERROR(k_log, "Material '{0}': unknown archetype '{1}' -- only 'standard_pbr' is implemented, packing it that way anyway",
            material.name, material.archetype);
    }

    CompiledMaterial compiled;
    compiled.archetype     = material.archetype;
    compiled.domain        = material.domain;
    compiled.double_sided  = material.double_sided;

    compiled.data.base_colour_factor  = material.base_color_factor;
    compiled.data.emissive_factor     = material.emissive_factor;
    compiled.data.metallic_factor     = material.metallic_factor;
    compiled.data.roughness_factor    = material.roughness_factor;
    compiled.data.normal_scale        = material.normal_scale;
    compiled.data.occlusion_strength  = material.occlusion_strength;
    compiled.data.alpha_cutoff        = material.alpha_cutoff;

    compiled.data.base_colour_texture         = rhi::kInvalidDescriptorIndex;
    compiled.data.normal_texture              = rhi::kInvalidDescriptorIndex;
    compiled.data.metallic_roughness_texture  = rhi::kInvalidDescriptorIndex;
    compiled.data.occlusion_texture           = rhi::kInvalidDescriptorIndex;
    compiled.data.emissive_texture            = rhi::kInvalidDescriptorIndex;
    compiled.data.flags = 0;
    compiled.data._pad0 = 0;
    compiled.data._pad1 = 0;

    compiled.texture_paths[Slot_BaseColor]         = ResolveTexturePath(model, material.base_color_texture, source_path);
    compiled.texture_paths[Slot_Normal]            = ResolveTexturePath(model, material.normal_texture, source_path);
    compiled.texture_paths[Slot_MetallicRoughness] = ResolveTexturePath(model, material.metallic_roughness_texture, source_path);
    compiled.texture_paths[Slot_Occlusion]         = ResolveTexturePath(model, material.occlusion_texture, source_path);
    compiled.texture_paths[Slot_Emissive]          = ResolveTexturePath(model, material.emissive_texture, source_path);

    return compiled;
}

MemoryBuffer phx::resources::SerializeMaterial(const CompiledMaterial& material, const std::string& source_path, const std::string& material_name)
{
    std::string string_table;
    string_table.push_back('\0');

    auto add_string = [&string_table](const std::string& s) -> u32
    {
        if (s.empty())
            return 0;

        const u32 offset = static_cast<u32>(string_table.size());
        string_table.append(s);
        string_table.push_back('\0');
        return offset;
    };

    const u32 source_path_offset = add_string(source_path);
    const u32 archetype_offset   = add_string(material.archetype);

    u32 texture_path_offsets[Slot_Count];
    for (u32 i = 0; i < Slot_Count; ++i)
        texture_path_offsets[i] = add_string(material.texture_paths[i]);

    u64 source_content_hash = 0;
    {
        MemoryBuffer source_bytes = VFS::ReadFile(source_path.c_str());
        if (!source_bytes.IsEmpty())
            source_content_hash = HashBytes(source_bytes.Data(), source_bytes.Size());
        else
            PHX_LOG_WARN(k_log, "Could not re-read source '{0}' (material '{1}') to compute content hash", source_path, material_name);
    }

    const u32 header_size           = static_cast<u32>(sizeof(ResourceFileHeader));
    const u32 chunk_table_size      = static_cast<u32>(sizeof(ChunkEntry));
    const u32 cpu_chunk_file_offset = AlignUp(header_size + chunk_table_size, 16u);
    const u32 cpu_chunk_size        = static_cast<u32>(sizeof(MaterialResource::CpuData));
    const u32 strings_file_offset   = AlignUp(cpu_chunk_file_offset + cpu_chunk_size, 16u);
    const u32 total_size            = strings_file_offset + static_cast<u32>(string_table.size());

    MemoryBuffer file_bytes(total_size, std::byte{ 0 });

    auto* header = reinterpret_cast<ResourceFileHeader*>(file_bytes.Data());
    header->magic               = kMaterialFileMagic;
    header->version              = kMaterialFileVersion;
    header->chunk_count          = 1;
    header->source_path_offset   = source_path_offset;
    header->source_content_hash  = source_content_hash;
    header->strings_offset       = strings_file_offset;
    header->strings_size         = static_cast<u32>(string_table.size());

    auto* chunks = reinterpret_cast<ChunkEntry*>(file_bytes.Data() + header_size);
    chunks[0] = ChunkEntry{ kMaterialChunkType_Cpu, cpu_chunk_file_offset, cpu_chunk_size };

    auto* cpu_data = reinterpret_cast<MaterialResource::CpuData*>(file_bytes.Data() + cpu_chunk_file_offset);
    cpu_data->data                  = material.data;
    cpu_data->archetype_name_offset = archetype_offset;
    for (u32 i = 0; i < Slot_Count; ++i)
        cpu_data->texture_path_offsets[i] = texture_path_offsets[i];
    cpu_data->domain = static_cast<u32>(material.domain);
    cpu_data->flags  = material.double_sided ? 1u : 0u;

    std::memcpy(file_bytes.Data() + strings_file_offset, string_table.data(), string_table.size());

    return file_bytes;
}

bool phx::resources::WriteMaterialFile(const char* virtual_path, const MemoryBuffer& file_bytes)
{
    return VFS::WriteFile(virtual_path,
        Span<const u8>(reinterpret_cast<const u8*>(file_bytes.Data()), file_bytes.Size()));
}
