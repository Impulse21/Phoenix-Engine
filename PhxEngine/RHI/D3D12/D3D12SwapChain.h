#pragma once

#include <D3D12Common.h>
#include <Core/RefCountPtr.h>
#include "D3D12Resources.h"

namespace PhxEngine::RHI
{
    struct SwapchainDesc;
    class Texture;
}

namespace PhxEngine::RHI::D3D12
{
    struct D3D12SwapChain
    {
        Core::RefCountPtr<IDXGISwapChain1> NativeSwapchain;
        Core::RefCountPtr<IDXGISwapChain4> NativeSwapchain4;
        std::vector<RHI::Texture> BackBuffers;
        std::vector<D3D12TypedCPUDescriptorHandle> BackBuferViews;

        const RHI::Texture& GetCurrentBackBuffer();

        explicit operator bool() const
        {
            return !!NativeSwapchain;
        }
    };
}

