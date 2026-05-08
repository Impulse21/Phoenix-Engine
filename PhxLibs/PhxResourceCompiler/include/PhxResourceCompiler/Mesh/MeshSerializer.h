#pragma once

#include <PhxCore/Result.h>
#include <PhxCore/IO/MemoryRegion.h>

namespace phx::resource::compiler
{
    struct BakedMesh;
}

namespace phx::resource::serializer
{
    Result<phx::MemoryBuffer> SerializeMesh(const compiler::BakedMesh& mesh);
}