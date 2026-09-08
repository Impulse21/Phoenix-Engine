#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Core/Result.h>
#include <PhxEngine/Resources/Intermediate/IntermediateModel.h>

namespace phx::resources
{
    Result<IntermediateModel> ImportGltfModel(const char* path);
}