#pragma once

#include <PhxEngine/RHI/PhxDynamicRHI.h>

namespace PhxEngine::RHI
{
    void Initialize(RHI::GraphicsAPI api);
    void Finiailize();

    DynamicRHI* GetDynamic();
}
