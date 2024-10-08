#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"

void phx::gfx::InitializeWindows(
	SwapChainDesc const& swapChainDesc,
	void* windowHandle)
{
	Ember::Ptr = new Ember();
	GfxDevice::Initialize(swapChainDesc, windowHandle);
}

void phx::gfx::Finalize()
{
	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}


phx::gfx::Ember::Ember() = default;

phx::gfx::Ember::~Ember()
{
	GfxDevice::Finalize();
}
