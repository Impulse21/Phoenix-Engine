#pragma once

#include <PhxEngine/Resources/MeshResource.h>
#include <PhxEngine/Core/RefCountPtr.h>

namespace phx
{
    struct IntermediateMesh;

    namespace compiler
    {
        struct Mesh
        {  
            // The single, interleaved vertex buffer for ALL submeshes.
            MemoryBuffer vertex_buffer;
            MemoryBuffer index_buffer;

            struct PrimitiveView
            {
                uint32_t index_count;
                uint32_t index_offset;
                uint32_t vertex_offset;
                
                // TODO: How the hell do I handle this bit?
                // Since this is going to disk, it will need to to know the assets name
                std::string material_id;
            };

            std::vector<PrimitiveView> primitives;


            RefCountPtr<MeshResource> ToResource();
        };

        Mesh CompileMesh(const IntermediateMesh& mesh);

    }
}