#pragma once


#ifdef PHX_VIRTUAL_DEVICE
#include "phxGfxCommandCtxInterface.h"
#else
#include "D3D12/phxCommandListD3D12.h"

#endif

namespace phx::gfx
{
    // Template-based compile-tiGfxBackendme selector
    template <GfxBackend api>
    class GfxCommandSelector;

    template <>
    class GfxCommandSelector<GfxBackend::Dx12>
    {
    public:
        using CommandListType = phx::gfx::platform::CommandListD3D12;
    };

#if false
    template <>
    class GfxCommandSelector<GfxBackend::Vulkan>
    {
    public:
        using CommandListType = phx::gfx::CommandListVulkan;
    };
#endif
}