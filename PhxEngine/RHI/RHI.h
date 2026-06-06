#pragma once

#include <PhxEngine/Core/Handle.h>
#include "RHITypes.h"

namespace phx::rhi
{
    bool Initialize();
    void Shutdown();

    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();
    
    SwapchainHandle CreateSwapchain(const SwapchainDesc& desc);
    void DeleteSwapchain(SwapchainHandle handle);
}