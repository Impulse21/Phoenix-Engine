#pragma once

#include <PhxCore/Result.h>
#include <PhxResourceCompiler/Mesh/MeshTypes.h>


struct cgltf_mesh;
struct cgltf_material;

namespace phx::resource::importer
{
    Result<compiler::RawMesh> ImportMesh(const cgltf_mesh& mesh, Span<cgltf_material> materials);
}