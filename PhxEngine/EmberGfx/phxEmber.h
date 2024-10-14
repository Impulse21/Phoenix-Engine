#pragma once

#include "phxPlatformDetection.h"
#include "phxGpuDeviceInterface.h"

namespace phx::gfx
{
	using GpuDevice = IGpuDevice;
	using CommandCtx = ICommandCtx;
	namespace EmberGfx
	{
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle);
		void Finalize();

		GpuDevice* GetDevice();
	}
}

