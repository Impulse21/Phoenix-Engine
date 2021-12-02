#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::RHI::Dx12
{
    namespace Factory
    {
        IGraphicsDevice* CreateDevice();
    }
    
}