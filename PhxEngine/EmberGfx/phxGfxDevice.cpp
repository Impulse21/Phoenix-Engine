#include "pch.h"
#include "phxGfxDevice.h"

using namespace phx::gfx;

void phx::gfx::GfxDeviceFactory::Create(GfxBackend backend, GfxDevice& device)
{
#ifdef PHX_VIRTUAL_DEVICE
	asset(false);
#else

#if defined(PHX_PLATFORM_WINDOWS)
	device.m_impl = std::make_unique<GfxDeviceD3D12>();
#endif

#endif
}
