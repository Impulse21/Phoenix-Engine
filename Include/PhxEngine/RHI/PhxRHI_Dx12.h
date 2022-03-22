#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <memory>

namespace PhxEngine::RHI::Dx12
{
    namespace Factory
    {
        std::unique_ptr<IGraphicsDevice> CreateDevice();
    }
    
}