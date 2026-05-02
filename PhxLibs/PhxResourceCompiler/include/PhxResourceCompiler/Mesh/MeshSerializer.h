#pragma once

#include <PhxCore/Result.h>
#include <PhxResourceCompiler/IntermediateMesh.h>

namespace phx::resource::serializer
{
    Result<void> SerializeMesh(const IntermediateMesh& mesh, const std::string& virtual_path);
}