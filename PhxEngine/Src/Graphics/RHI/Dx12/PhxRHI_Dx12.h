#pragma once

#include "PhxEngine/Graphics/RHI/PhxRHI.h"
#include <memory>

namespace PhxEngine::RHI
{
    IGraphicsDevice* CreateDx12Device();
}
namespace PhxEngine::RHI::Dx12
{
    namespace Factory
    {
        std::unique_ptr<IGraphicsDevice> CreateDevice();
    }
    
}