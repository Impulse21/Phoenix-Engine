#pragma once

#include "IntermediateMesh.h"
#include "IntermediateTexture.h"

#include <vector>

namespace phx
{
    struct IntermediateModel
    {
        std::vector<IntermediateMesh> meshes;
        std::vector<IntermediateTextures> textures;
    }
}