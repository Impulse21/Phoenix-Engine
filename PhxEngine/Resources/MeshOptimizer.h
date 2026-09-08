#pragma once

namespace phx
{
    struct IntermediateMesh;
    namespace MeshOptimizer
    {
        bool Optimize(IntermediateMesh& mesh);
    }
}