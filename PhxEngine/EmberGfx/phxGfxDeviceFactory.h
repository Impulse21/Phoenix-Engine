#pragma once
#include "phxGfxDevice.h"

namespace phx::gfx
{
	class GfxDeviceFactory
	{
	public:
		static GfxDevice* Create(GfxApi api);
	};
}