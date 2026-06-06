#pragma once

#include <PhxEngine/Core/Handle.h>
#include "RHITypes.h"

namespace phx::rhi
{
    bool Initialize(const char* app_name);
    void Shutdown();

    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();
    
    SwapchainHandle CreateSwapchain(const SwapchainDesc& desc);
    void DeleteSwapchain(SwapchainHandle handle);
}