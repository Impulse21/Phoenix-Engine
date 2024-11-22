#pragma once

#include "phx/rhi/RHITypes.h"

namespace phx
{
	namespace Display
	{
		void Initialize(rhi::SwapChainDescriptor const& swapchainDesc);
		void Finalize();

		void Resize(uint32_t width, uint32_t height);

		extern rhi::SwapChainDescriptor g_SwapChainDesc;
		extern rhi::SwapChainHandle g_SwapChain;
	}
}