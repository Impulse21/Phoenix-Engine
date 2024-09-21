#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDeviceFactory.h"

void phx::gfx::InitializeWindows()
{
	Ember::Ptr = new Ember();
	Ember::Ptr->GfxDevice = GfxDeviceFactory::Create(GfxApi::DX12);
}

void phx::gfx::Shutdown()
{
	delete Ember::Ptr->GfxDevice;
	Ember::Ptr->GfxDevice = nullptr;

	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}
