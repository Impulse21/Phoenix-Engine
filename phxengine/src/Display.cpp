#include "pch.h"
#include "phx/Display.h"

#include "phx/rhi/GfxDevice.h"

using namespace phx;
using namespace phx::gfx;

namespace phx::gfx
{
	rhi::SwapChainDescriptor g_SwapChainDesc;
	rhi::SwapChainHandle g_SwapChain;
}

void Display::Initialize(rhi::SwapChainDescriptor const& swapchainDesc)
{
	auto* device = rhi::GfxDevice::Ptr;
	g_SwapChainDesc = swapchainDesc;
	g_SwapChain = device->CreateSwapChain(g_SwapChainDesc);
}

void Display::Finalize()
{
	auto* device = rhi::GfxDevice::Ptr;
	device->DeleteSwapChain(g_SwapChain);
}