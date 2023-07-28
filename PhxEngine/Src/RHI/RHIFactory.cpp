#include <PhxEngine/RHI/RHICore.h>

#include "GfxDriver.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Factory;

namespace
{
}

bool PhxEngine::RHI::Factory::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& swapchain)
{
	return RHI::GfxDriver::Impl->CreateSwapChain(desc, windowHandle, swapchain);
}
