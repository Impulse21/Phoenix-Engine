#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"
#include "phxGfxDeviceSelector.h"

namespace phx::gfx
{
#ifdef PHX_VIRTUAL_DEVICE
	using GfxDevice = IGfxDevice;
#else
	using GfxDevice = GfxDeviceSelector<kSelectedAPI>::GfxDeviceType;
#endif

}