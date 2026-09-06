#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/RelativePtr.h>

#include <PhxEngine/RHI/RHITypes.h>

#include "Resource.h"

namespace phx
{

    struct MeshResource : public Resource
    {
        PHX_DECLARE_RESOURCE(MeshResource);

        struct CpuData
        {
            struct DrawInfo
            {
                u32 prim_count;
                u32 start_index;
                u32 base_vertex;
            };

            u32 index_data_offset;
            u32 index_data_size;

            u32 vertex_data_offset;
            u32 vertex_data_size;

            RelativePtr<DrawInfo> draw_info;
            u32 draw_info_count;
        };

        // This is not needed right but, will be useful with we 
        // load from our own pack mesh file.
        phx::MemoryBuffer cpu_data_buffer;
        TypedView<CpuData> cpu_data;

        rhi::GpuAllocation packed_mesh_buffer;
    };
}