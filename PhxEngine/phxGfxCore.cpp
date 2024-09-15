#include "pch.h"
#include "phxGfxCore.h"
#include "phxDisplay.h"
#include "phxGfxPlatformDevice.h"

using namespace phx;

namespace phx::gfx
{
	void Initialize()
	{
		if (Device::Ptr)
			return;

		Device::Ptr = new Device();
		Device::Ptr->Initialize();

		// TODO: Initialize Common Types
		Display::Initialize();

		// Add GPU Time Manager

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

	void SwapChain::Initialize(SwapChainDesc desc)
	{
		this->m_desc = desc;
		Device::Ptr->GetPlatform().Create(desc, this->m_platformResource);
	}

}