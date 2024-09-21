#pragma once

#ifdef PHX_VIRTUAL_DEVICE
#include "phxGfxDeviceInterface.h"
#else
#include "phxGfxDevice.h"
#include "D3D12/phxGfxDeviceD3D12.h"
#include "Vulkan/phxGfxDeviceVulkan.h"

#endif

namespace phx::gfx
{
#ifdef PHX_VIRTUAL_DEVICE
    // Template-based compile-time selector
    template <GfxApi api>
    class GfxDeviceSelector
    {
    public:
        using GfxDeviceType = phx::gfx::IGfxDevice;
    };
#else
    // Template-based compile-tiGfxBackendme selector
    template <GfxBackend api>
    class GfxDeviceSelector;

    template <>
    class GfxDeviceSelector<GfxBackend::Dx12>
    {
    public:
        using GfxDeviceType = phx::gfx::GfxDeviceD3D12;
    };

    template <>
    class GfxDeviceSelector<GfxBackend::Vulkan>
    {
    public:
        using GfxDeviceType = phx::gfx::GfxDeviceVulkan;
    };
#endif
}