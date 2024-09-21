#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDeviceFactory.h"

void phx::gfx::InitializeWindows()
{
	Ember::Ptr = new Ember();
	Ember::Ptr->GfxDevice = GfxDeviceFactory::Create(GfxBackend::Dx12);
}

void phx::gfx::Finalize()
{
	delete Ember::Ptr->GfxDevice;
	Ember::Ptr->GfxDevice = nullptr;

	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}
