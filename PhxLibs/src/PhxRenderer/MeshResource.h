#pragma once

#include <PhxCore/IO/MemoryRegion.h>

#include <PhxResource/ResourceFwds.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxResource/FileFormatUtils.h>

#include <PhxRhi/PhxRhi.h>

namespace phx::renderer
{
    struct MeshResource final : public ResourceHotData
    {
        struct CpuData
        {
            struct Draw
            {
                uint32_t prim_count;    // Number of indices = 3 * number of triangles
                uint32_t start_index;   // Offset to first index in index buffer
                uint32_t base_vertex;   // Offset to first vertex in vertex buffer
            };

            float							bounding_sphere[4];
            float							bounding_box[6];
            uint32_t						index_data_offset;
            uint32_t						index_data_size;
            uint32_t                        vertex_data_offset;
            uint32_t                        vertex_data_size;
            FileFormat::RelativePtr<Draw>	draws;
            uint32_t						num_draws;
        };

        phx::MemoryBuffer cpu_data_buffer;
        TypedView<CpuData> cpu_data;

        rhi::BufferHandle packed_mesh_buffer;

        ~MeshResource();
    };
    static_assert(sizeof(MeshResource) <= 64);

    struct MeshColdData final : public ResourceColdData
    {

    };
}

namespace phx::mesh_ops
{
    bool CollectPendingGpuTransitions(GenericHandle handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index);
}

PHX_DEFINE_RESOURCE(
    renderer::MeshResource,    // T
    renderer::MeshResource,     // Hot
    renderer::MeshColdData,    // Cold
    ".phxmsh",                 // Extension
    "MeshLoader"               // Loader ID
);