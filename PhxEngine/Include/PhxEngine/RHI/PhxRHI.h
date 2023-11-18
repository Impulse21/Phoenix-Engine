#pragma once

#include <PhxEngine/RHI/PhxRHIResources.h>
#include <PhxEngine/RHI/PhxDynamicRHI.h>

namespace PhxEngine::RHI
{
    void Initialize(RHI::GraphicsAPI api);
    void Finiailize();

    DynamicRHI* GetDynamic();
}

namespace std
{
    // TODO: Custom Hashes
}