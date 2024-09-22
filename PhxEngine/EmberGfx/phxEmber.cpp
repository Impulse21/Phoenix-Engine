#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDeviceFactory.h"

void phx::gfx::InitializeWindows(
	GfxBackend backend,
	SwapChainDesc const& swapChainDesc,
	void* windowHandle)
{
	Ember::Ptr = new Ember();
	Ember::Ptr->GfxDevice = GfxDeviceFactory::Create(backend);
	Ember::Ptr->GfxDevice->Initialize(swapChainDesc, windowHandle);
}

void phx::gfx::Finalize()
{
	delete Ember::Ptr->GfxDevice;
	Ember::Ptr->GfxDevice = nullptr;

	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}
