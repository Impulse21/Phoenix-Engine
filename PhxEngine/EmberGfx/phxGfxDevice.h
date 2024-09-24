#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"
#include "phxGfxDeviceSelector.h"

namespace phx::gfx
{
#ifndef PHX_VIRTUAL_DEVICE
#if defined(_WIN32)
	constexpr GfxBackend kSelectedAPI = GfxBackend::Dx12;
#endif
#endif
		

#ifdef PHX_VIRTUAL_DEVICE
	using CommandList = ICommandList;
	using GfxDevice = IGfxDevice;
#else
	using GfxDevice = GfxDeviceSelector<kSelectedAPI>::CommandListType;
	using GfxDevice = GfxDeviceSelector<kSelectedAPI>::GfxDeviceType;
#endif

}