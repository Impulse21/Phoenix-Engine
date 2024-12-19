#include "phx/pch.h"
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

void Display::Present()
{
	auto* device = rhi::GfxDevice::Ptr;
	device->Present({ g_SwapChain });
}

void Display::Resize(uint32_t width, uint32_t height)
{
	auto* device = rhi::GfxDevice::Ptr;
	g_SwapChainDesc.Width = width;
	g_SwapChainDesc.Height = height;

	device->CreateSwapChain(g_SwapChainDesc, g_SwapChain);
}

void Display::Finalize()
{
	auto* device = rhi::GfxDevice::Ptr;
	device->DeleteSwapChain(g_SwapChain);
}