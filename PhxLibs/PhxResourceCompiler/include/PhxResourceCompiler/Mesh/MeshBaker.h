#pragma once

namespace phx::resource::baker
{
    struct RawMesh;
    struct BakedMesh;

    bool BakeMesh(const RawMesh& raw_mesh, BakedMesh& out_baked_mesh);
    
}