#pragma once

#include <PhxResource/Resource.h>

namespace phx::renderer
{
    struct MeshResource final : public Resource
    {
#if false
        // Handles to the single, large GPU buffers.
        GpuBufferHandle vertexBuffer;
        GpuBufferHandle indexBuffer;

        struct SubMesh
        {
            uint32_t indexCount;
            uint32_t indexOffset;
            uint32_t vertexOffset;

            RefCountPtr<Resource> material; // Handle to the loaded material
            math::AxisAlignedBox boundingBox;
        };

        std::vector<SubMesh> subMeshes;
#endif
    };
}

