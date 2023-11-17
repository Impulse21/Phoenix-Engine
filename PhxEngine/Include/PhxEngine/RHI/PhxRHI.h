#pragma once

#include <PhxEngine/RHI/PhxRHIResources.h>
#include <PhxEngine/RHI/PhxDynamicRHI.h>

namespace PhxEngine::RHI
{
    void Initialize();
    void Finiailize();

    DynamicRHI* GetRHI();
}

namespace std
{
    // TODO: Custom Hashes
}