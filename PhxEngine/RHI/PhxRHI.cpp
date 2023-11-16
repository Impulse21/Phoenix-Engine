#include <RHI/PhxRHI.h>

#include <PlatformTypes.h>

using namespace PhxEngine::RHI;

bool PhxEngine::RHI::Initialize(RHIParams const& params)
{
	return PlatformGfxDevice::Initialize();
}

bool PhxEngine::RHI::Finalize()
{
	PlatformGfxDevice::Finalize();
	return true;
}

void PhxEngine::RHI::Present(SwapChain const& swapchainToPresent)
{
	PlatformGfxDevice::Present(swapchainToPresent);

}

void PhxEngine::RHI::WaitForIdle()
{
	PlatformGfxDevice::WaitForIdle();
}

bool PhxEngine::RHI::Factory::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& out)
{
	out.m_Desc = desc;
	return PlatformGfxDevice::CreateSwapChain(desc, windowHandle, out.m_PlatfomrSwapChain);

}

bool PhxEngine::RHI::Factory::CreateTexture(TextureDesc const& desc, Texture& out)
{
	out.m_Desc = desc;
	return PlatformGfxDevice::CreateTexture(desc, out.m_PlatfomrTexture);

}