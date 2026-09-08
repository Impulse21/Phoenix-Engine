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

            // Stable name to key material cooking off later; see
            // IntermediateMesh::Primitive::material_name.
            std::string material_name;
        };

        std::vector<PrimitiveView> primitives;
    };

    CompiledMesh CompileMesh(const IntermediateMesh& mesh);

    // Packs a CompiledMesh into a .phxmsh-shaped in-memory blob (see
    // MeshFileFormat.h) -- no disk access. `source_path` is a VFS virtual
    // path, re-read here to compute the provenance content hash; `mesh_name`
    // is used only for log messages (it's already encoded in the cooked
    // file's name via CookedMeshPath, so it isn't persisted separately).
    MemoryBuffer SerializeMesh(const CompiledMesh& mesh, const std::string& source_path, const std::string& mesh_name);

    // Thin VFS::WriteFile wrapper.
    bool WriteMeshFile(const char* virtual_path, const MemoryBuffer& file_bytes);
}