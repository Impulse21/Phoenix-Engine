#pragma once

namespace phx::resource::compiler
{
    struct RawMesh;
}

namespace phx::resource::optimizer
{
    void OptimizeMesh(compiler::RawMesh& mesh);
}