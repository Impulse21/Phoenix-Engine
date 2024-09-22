#pragma once

#include "phxGfxDeviceResources.h"

namespace phx::gfx
{
	class IGfxDevice
	{
	public:
		virtual ~IGfxDevice() = default;

	public:
		virtual void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr) = 0;
		virtual void Finalize() = 0;

		virtual void WaitForIdle() = 0;
		virtual void ResizeSwapChain(SwapChainDesc const& swapChainDesc) = 0;
	};
}