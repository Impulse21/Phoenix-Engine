#include "MeshOptimizer.h"

#include <meshoptimizer.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Resources/Intermediate/IntermediateMesh.h>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "Optimizer" };
}

bool phx::resources::OptimizeMesh(IntermediateMesh& mesh)
{
    PHX_UNUSED(mesh);
    PHX_LOG_WARN(k_log, "Mesh Optimization is not implemented yet...");
    return true;
}