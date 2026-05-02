#pragma once

#include <PhxCore/Result.h>
#include <PhxResourceCompiler/Mesh/MeshImporter_Gltf.h>
#include <PhxResourceCompiler/Mesh/MeshOptimizer.h>
#include <PhxResourceCompiler/Mesh/MeshSerializer.h>

struct cgltf_mesh;

namespace phx::resource::compiler
{
    Result<void> CompileMesh(const cgltf_mesh& mesh, const std::string& virtual_path)
    {
        Result<IntermediateMesh> import_result = importer::ImportMesh(mesh);
        if (!import_result)
            return Unexpected(import_result.GetError());

        IntermediateMesh& intermediate_mesh = import_result.GetValue();
        
        optimizer::OptimizeMesh(intermediate_mesh);

        return serializer::SerializeMesh(intermediate_mesh, virtual_path);
    }
}