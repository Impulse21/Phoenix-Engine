#pragma once

#include <RHI/RHIResources.h>
#include <PlatformTypes.h>
#include <RHI/RHIEnums.h>
#include <Core/Memory.h>
#include <Core/Span.h>

namespace PhxEngine::RHI
{
	struct RHIParams
	{
		size_t NumInflightFrames = 3;
		size_t ResourcePoolSize = PhxKB(1);
	};

	bool Initialize(RHIParams const& params);
	bool Finalize();

	// Singles the end of the frame.
	// Presents provided swapchains,
	void Present(SwapChain const& swapchainsToPresent);
	void WaitForIdle();


	class Factory
	{
	public:

		static bool CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& out);
		static bool CreateTexture(TextureDesc const& desc, Texture& out);
	};
}