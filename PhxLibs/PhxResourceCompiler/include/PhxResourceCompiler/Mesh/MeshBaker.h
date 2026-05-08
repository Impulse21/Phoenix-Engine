#pragma once

namespace phx::resource::compiler
{
    struct RawMesh;
    struct BakedMesh;
}

namespace phx::resource::baker
{
    bool BakeMesh(const compiler::RawMesh& raw_mesh, compiler::BakedMesh& out_baked_mesh);
    
}