#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace PhxEngine::RHI
{
	class GfxDriver
	{
	public:
		inline static GfxDriver* Impl;

	public:
		virtual ~GfxDriver() = default;

		virtual void Initialize() = 0;
		virtual void Finialize() = 0;
		virtual void WaitForIdle() = 0;
		// Frame Functions
	public:

		// Create Functions
	public:
		virtual bool CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& swapchain) = 0;
	};

}