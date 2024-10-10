#pragma once

#include "phxGfxDevice.h"
#include "phxPlatformDetection.h"

namespace phx::gfx
{
#if defined(PHX_PLATFORM_WINDOWS)
	void InitializeWindows(SwapChainDesc const& swapCahinDesc, void* windowHandle);
#endif
	void Finalize();


#if false
	class Ember
	{
	public:
		inline static Ember* Ptr = nullptr;

	public:
		Ember();
		~Ember();
		GfxDevice& GetDevice() { return this->m_gfxDevice; }

	private:
		GfxDevice m_gfxDevice;
	};
#endif
}

