#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Core/Result.h>
#include <PhxEngine/Resources/Intermediate/IntermediateModel.h>

namespace phx
{
    namespace AssetImporter
    {
        Result<IntermediateModel> ImportGltfModel(const char* path);
    }
}