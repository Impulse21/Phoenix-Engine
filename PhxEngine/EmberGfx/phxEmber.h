#pragma once

#include "phxGfxDevice.h"
#include "phxPlatformDetection.h"

#include "phxGpuDeviceInterface.h"

namespace phx::gfx
{
	using GpuDevice = IGpuDevice;
	namespace EmberGfx
	{
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle);
		void Finalize();

		GpuDevice* GetDevice();
	}
}

