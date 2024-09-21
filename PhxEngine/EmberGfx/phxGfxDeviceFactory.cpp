#include "pch.h"
#include "phxGfxDeviceFactory.h"

#ifdef PHX_VIRTUAL_DEVICE
#include "phxGfxDeviceD3D12.h"
#include "phxGfxDeviceVulkan.h"
#endif

phx::gfx::GfxDevice* phx::gfx::GfxDeviceFactory::Create(GfxApi api)
{
#ifdef PHX_VIRTUAL_DEVICE
    switch (api)
    {
    case GfxApi::DX12:
        return new GfxDeviceD3D12();
    case GfxApi::Vulkan:
        return new GfxDeviceVulkan();
    default:
        break;
    }
    return nullptr;
#else
    return new GfxDevice();
#endif
}
