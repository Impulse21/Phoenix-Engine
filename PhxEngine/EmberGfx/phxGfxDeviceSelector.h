#pragma once

#ifndef PHX_VIRTUAL_DEVICE

#include "phxGfxDevice.h"
#include "phxGfxDeviceD3D12.h"
#include "phxGfxDeviceVulkan.h"
namespace phx::gfx
{

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
}

#endif