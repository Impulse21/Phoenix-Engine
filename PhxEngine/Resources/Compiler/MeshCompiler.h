#pragma once

#include <PhxEngine/Resources/MeshResource.h>
#include <PhxEngine/Core/RefCountPtr.h>

namespace phx::resources
{
    struct IntermediateMesh;

    struct CompiledMesh
    {
        // The single, interleaved vertex buffer for ALL submeshes.
        MemoryBuffer vertex_buffer;
        MemoryBuffer index_buffer;

        struct PrimitiveView
        {
            uint32_t index_count;
            uint32_t index_offset;
            uint32_t vertex_offset;
            std::string material_name;
        };

        std::vector<PrimitiveView> primitives;
    };

    CompiledMesh CompileMesh(const IntermediateMesh& mesh);
    MemoryBuffer SerializeMesh(const CompiledMesh& mesh, const std::string& source_path, const std::string& mesh_name);
    bool WriteMeshFile(const char* virtual_path, const MemoryBuffer& file_bytes);
}
