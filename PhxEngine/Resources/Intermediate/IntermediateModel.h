#pragma once

#include "IntermediateMesh.h"
#include "IntermediateTexture.h"
#include "IntermediateModel.h"

#include <vector>

namespace phx
{
    struct IntermediateModel
    {
        std::vector<IntermediateMesh> meshes;
        std::vector<IntermediateTexture> textures;
    };
}