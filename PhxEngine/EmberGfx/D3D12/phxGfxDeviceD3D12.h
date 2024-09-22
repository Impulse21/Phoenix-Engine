#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

namespace phx::gfx
{
	class GfxDeviceD3D12 final : public IGfxDevice
	{
	public:
		GfxDeviceD3D12() = default;
		~GfxDeviceD3D12() = default;

		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr) override;
		void Finalize() override;

		void WaitForIdle() override;
		void ResizeSwapChain(SwapChainDesc const& swapChainDesc) override;
	};
}

