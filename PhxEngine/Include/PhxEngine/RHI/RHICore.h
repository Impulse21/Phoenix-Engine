#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace PhxEngine::RHI
{
	enum class GraphicsAPI
	{
		Unknown = 0,
		DX12
	};

	struct RHIParmaeters
	{
		GraphicsAPI Api = GraphicsAPI::DX12;
		bool EnableDebug = true;

	};

	bool Initialize(RHIParmaeters const& parameters);
	void Finalize();
	
	void Present(const SwapChain& Swapchain);
	void RunGarbageCollection();

	void WaitForIdle();

	namespace Factory
	{
		bool CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& swapchain);

	}
}