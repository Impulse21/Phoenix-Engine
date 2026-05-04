#pragma once

#include <PhxCore/Result.h>
#include <PhxResourceCompiler/Mesh/MeshImporter_Gltf.h>
#include <PhxResourceCompiler/Mesh/MeshOptimizer.h>
#include <PhxResourceCompiler/Mesh/MeshSerializer.h>

#include <PhxresourceMesh/MeshTypes.h>

struct cgltf_mesh;

namespace phx::resource::compiler
{
    struct RawMesh;
    struct BakedMesh;

    Result<void> CompileMesh(const cgltf_mesh& mesh, const std::string& virtual_path)
    {
        Result<RawMesh> import_result = importer::ImportMesh(mesh);
        if (!import_result)
            return Unexpected(import_result.GetError());

        RawMesh& raw_mesh = import_result.GetValue();
        
        optimizer::OptimizeMesh(raw_mesh);

        BakedMesh baked_mesh;
        if (!baker::BakeMesh(raw_mesh, baked_mesh))
            return Unexpected(ResultError::Failure);

        return serializer::SerializeMesh(baked_mesh, virtual_path);
    }
}