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
    // Template-based compile-time selector
    template <GfxApi api>
    class GfxDeviceSelector;

    template <>
    class GfxDeviceSelector<GfxApi::DX12>
    {
    public:
        using GfxDeviceType = phx::gfx::GfxDeviceD3D12;
    };

    template <>
    class GfxDeviceSelector<GfxApi::Vulkan>
    {
    public:
        using GfxDeviceType = phx::gfx::GfxDeviceVulkan;
    };
#endif
}