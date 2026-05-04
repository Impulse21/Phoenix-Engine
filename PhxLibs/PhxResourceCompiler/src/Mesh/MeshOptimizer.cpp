#include "PhxResource_pch.h"

#include <PhxResourceCompiler/Mesh/MeshOptimizer.h>

using namespace phx::resource;
using namespace phx::resource::compiler;

void optimizer::OptimizeMesh(IntermediateMesh& /*mesh*/)
{
    // no-op for now, but this is where we would perform
    //  vertex deduplication, index buffer optimization, etc.
}