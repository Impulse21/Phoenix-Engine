#pragma once

#include <PhxCore/Result.h>

#include <PhxResourceCompiler/Mesh/MeshTypes.h>

#include <PhxResourceCompiler/Mesh/MeshImporter_Gltf.h>
#include <PhxResourceCompiler/Mesh/MeshOptimizer.h>
#include <PhxResourceCompiler/Mesh/MeshSerializer.h>
#include <PhxResourceCompiler/Mesh/MeshBaker.h>

struct cgltf_mesh;

namespace phx::resource::compiler
{
    struct RawMesh;
    struct BakedMesh;

    Result<MemoryBuffer> CompileMesh(const cgltf_mesh& mesh, Span<cgltf_material> materials)
    {
        using namespace phx;
        using namespace phx::resource;

        Result<RawMesh> import_result = importer::ImportMesh(mesh, materials);
        if (!import_result)
            return Unexpected(import_result.GetError());

        RawMesh& raw_mesh = import_result.GetValue();
        
        optimizer::OptimizeMesh(raw_mesh);

        BakedMesh baked_mesh;
        if (!baker::BakeMesh(raw_mesh, baked_mesh))
            return Unexpected(ResultError::Failure);

        return serializer::SerializeMesh(baked_mesh);
    }
}