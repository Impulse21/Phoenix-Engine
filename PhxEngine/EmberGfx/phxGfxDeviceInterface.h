#pragma once

#include "phxGfxDeviceResources.h"

#include "phxGfxCommandCtx.h"

namespace phx::gfx
{
	constexpr size_t kBufferCount = 3;
		
	class IGfxDevice
	{
	public:
		virtual ~IGfxDevice() = default;

	public:
		virtual void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr) = 0;
		virtual void Finalize() = 0;

		virtual void WaitForIdle() = 0;
		virtual void ResizeSwapChain(SwapChainDesc const& swapChainDesc) = 0;

        virtual CommandList& BeginGfxContext() = 0;
        virtual CommandList& BeginComputeContext() = 0;

        virtual void SubmitFrame() = 0;
	};
}