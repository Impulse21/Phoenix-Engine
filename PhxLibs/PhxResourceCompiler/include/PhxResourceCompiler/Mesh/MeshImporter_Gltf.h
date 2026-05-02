#pragma once

#include <PhxCore/Result.h>
#include <PhxResourceCompiler/Mesh/IntermediateMesh.h>


struct cgltf_mesh;

namespace phx::resource::importer
{
    Result<compiler::IntermediateMesh> ImportMesh(const cgltf_mesh& mesh);
}