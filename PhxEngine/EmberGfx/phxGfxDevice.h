#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

#ifdef PHX_VIRTUAL_DEVICE
#include "EmberGfx/phxGfxDeviceInterface.h"
#else
#include "phxGfxDeviceSelector.h"
#endif

namespace phx::gfx
{
#ifndef PHX_VIRTUAL_DEVICE
#if defined(_WIN32)
	constexpr GfxApi kSelectedAPI = GfxApi::DX12;
#endif
#endif


#ifdef PHX_VIRTUAL_DEVICE
	using GfxDevice = IGfxDevice;
#else
	using GfxDevice = GfxDeviceSelector<kSelectedAPI>::GfxDeviceType;
#endif

}