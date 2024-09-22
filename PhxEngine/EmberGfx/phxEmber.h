#pragma once

#include "phxGfxDevice.h"
#include "phxPlatformDetection.h"

namespace phx::gfx
{
#if defined(PHX_PLATFORM_WINDOWS)
	void InitializeWindows(GfxBackend backend, SwapChainDesc const& swapCahinDesc, void* windowHandle);
#endif
	void Finalize();

	class Ember
	{
	public:
		inline static Ember* Ptr = nullptr;

		GfxDevice* GfxDevice = nullptr;
	};
}

