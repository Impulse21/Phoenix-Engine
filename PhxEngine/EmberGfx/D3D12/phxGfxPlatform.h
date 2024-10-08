#pragma once

#include "phxCommandCtxD3D12.h"
#include "phxGfxDeviceD3D12.h"

namespace phx::gfx
{
	using PlatformGfxDevice = phx::gfx::GfxDeviceD3D12;
	using PlatformCommandCtx = phx::gfx::platform::CommandCtxD3D12;
}