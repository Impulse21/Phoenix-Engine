#include "pch.h"
#include "phxGfxCore.h"
#include "phxDisplay.h"
#include "phxGfxDevice.h"

using namespace phx;

namespace phx::gfx
{
	void Initialize()
	{
		if (Device::Ptr)
			return;

		Device::Ptr = new Device();
		Device::Ptr->Initialize();

		Display::Initialize();

		// Initialize Other gfx Systems
	}

	void Finalize()
	{
		if (!Device::Ptr)
			return;

		Device::Ptr->Finalize();
		delete Device::Ptr;

		Device::Ptr = nullptr;
	}
}