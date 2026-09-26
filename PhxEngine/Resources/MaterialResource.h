#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/Renderer/Shaders/Interop.h>

#include "Resource.h"
#include "TextureResource.h"
#include "MeshResource.h"

namespace phx::resources
{
    enum MaterialTextureSlot
    {
        Slot_BaseColor,
        Slot_Normal,
        Slot_MetallicRoughness,
        Slot_Occlusion,
        Slot_Emissive,
        Slot_Count,
    };

    struct MaterialResource : public Resource
    {
        PHX_DECLARE_RESOURCE(MaterialResource);

        struct CpuData
        {
            renderer::MaterialData data;

            u32 archetype_name_offset;
            u32 texture_path_offsets[Slot_Count];
            u32 domain;
            u32 flags;
        };

        MemoryBuffer cpu_data_buffer;
        TypedView<CpuData> cpu_data;

        RefCountPtr<TextureResource> textures[Slot_Count];
        renderer::MaterialData gpu_data;
    };

    RefCountPtr<MaterialResource> CreateMaterialResource(MemoryBuffer&& file_bytes);
    RefCountPtr<MaterialResource> LoadMaterialResource(const char* virtual_path);
    RefCountPtr<MaterialResource> LoadMaterialForDrawInfo(const MeshResource& mesh, const MeshResource::CpuData::DrawInfo& draw_info);
}
