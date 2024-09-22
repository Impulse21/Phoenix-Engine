#include "pch.h"
#include "phxGfxDeviceFactory.h"

#include "phxGfxDeviceSelector.h"

using namespace phx::gfx;

phx::gfx::GfxDevice* phx::gfx::GfxDeviceFactory::Create(GfxBackend api)
{
#ifdef PHX_VIRTUAL_DEVICE
    switch (api)
    {
    case GfxApi::DX12:
        return  GfxDeviceSelector<GfxApi::DX12>::GfxDeviceType();
#if false
    case GfxApi::Vulkan:
        return GfxDeviceSelector<GfxApi::Vulkan>::GfxDeviceType();
#endif
    default:
        break;
    }
    return nullptr;
#else
    return new GfxDevice();
#endif
}
