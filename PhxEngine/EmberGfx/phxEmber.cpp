#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"

void phx::gfx::InitializeWindows(
	GfxBackend backend,
	SwapChainDesc const& swapChainDesc,
	void* windowHandle)
{
	Ember::Ptr = new Ember(GfxBackend::Dx12);
	Ember::Ptr->GetDevice().Initialize(swapChainDesc, windowHandle);
}

void phx::gfx::Finalize()
{
	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}


phx::gfx::Ember::Ember(GfxBackend backend)
{
	GfxDeviceFactory::Create(backend, this->m_gfxDevice);
}

phx::gfx::Ember::~Ember()
{
	this->m_gfxDevice.Finalize();
}
