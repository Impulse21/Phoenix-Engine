#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/RelativePtr.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/RHI/RHITypes.h>

#include "Resource.h"

namespace phx::resources
{
    struct MeshResource : public Resource
    {
        PHX_DECLARE_RESOURCE(MeshResource);

        struct CpuData
        {
            struct DrawInfo
            {
                u32 index_count;
                u32 index_byte_offset;
                u32 stream_header_byte_offset;
                u32 material_name_offset;
            };

            RelativePtr<DrawInfo> draw_info;
            u32 draw_info_count;
        };

        phx::MemoryBuffer cpu_data_buffer;
        TypedView<CpuData> cpu_data;

        rhi::GpuRange packed_mesh_buffer;

        void Dispose() override;
    };

    RefCountPtr<MeshResource> CreateMeshResource(MemoryBuffer&& file_bytes);
    RefCountPtr<MeshResource> LoadMeshResource(const char* virtual_path);
}
