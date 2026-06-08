#pragma once

#include <PhxEngine/Core/Handle.h>
#include "RHITypes.h"

namespace phx::rhi
{
    struct InitParam
    {
        const char* app_name = nullptr;

        bool enable_validation          = false;
        bool enable_best_practices      = true;
        bool enable_sync_validation     = false;
        bool enable_gpu_assisted        = false; // Very Expensive
    };

    bool Initialize(const InitParam& param);
    void Shutdown();

    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();
    
    SwapchainHandle CreateSwapchain(const SwapchainDesc& desc);
    void DeleteSwapchain(SwapchainHandle handle);
}