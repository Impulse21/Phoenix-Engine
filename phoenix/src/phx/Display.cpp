#include "phxpch.h"
#include "phx/Display.h"


using namespace phx;
using namespace phx::gfx;

namespace phx::gfx
{
	rhi::SwapChainDescriptor g_SwapChainDesc;
	rhi::SwapChainHandle g_SwapChain;
}

void Display::Initialize(rhi::SwapChainDescriptor const& swapchainDesc)
{
#if false
	auto* device = rhi::GfxDevice::Ptr;
	g_SwapChainDesc = swapchainDesc;
	g_SwapChain = device->CreateSwapChain(g_SwapChainDesc);
#endif
}

void Display::Present()
{
#if false
	auto* device = rhi::GfxDevice::Ptr;
	device->Present({ g_SwapChain });
#endif
}

void Display::Resize(uint32_t width, uint32_t height)
{
#if false
	auto* device = rhi::GfxDevice::Ptr;
	g_SwapChainDesc.Width = width;
	g_SwapChainDesc.Height = height;

	device->CreateSwapChain(g_SwapChainDesc, g_SwapChain);
#endif
}

void Display::Finalize()
{
#if false
	auto* device = rhi::GfxDevice::Ptr;
	device->DeleteSwapChain(g_SwapChain);
#endif
}
