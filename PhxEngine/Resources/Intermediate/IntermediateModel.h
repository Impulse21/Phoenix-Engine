#pragma once

#include "IntermediateMesh.h"
#include "IntermediateTexture.h"
#include "IntermediateMaterials.h"

#include <vector>

namespace phx::resources
{
    struct IntermediateModel
    {
        std::vector<IntermediateMesh> meshes;
        std::vector<IntermediateTexture> textures;
        std::vector<IntermediateMaterial> materials;
    };
}