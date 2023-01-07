#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <memory>

namespace PhxEngine::RHI
{
    IGraphicsDevice* CreateDx12Device();
}

namespace PhxEngine::RHI::D3D12
{
    namespace Factory
    {
        std::unique_ptr<IGraphicsDevice> CreateDevice();
    }
    
}