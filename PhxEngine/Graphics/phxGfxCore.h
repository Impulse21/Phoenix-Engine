#pragma once

#include "phxGfxDescriptors.h"

#include "D3D12/phxGfxPlatformTypes.h"

namespace phx::gfx
{
	struct IDeviceNotify;
	void Initialize(ContextDesc const& desc)
	{
		PlatformContext::Initialize(desc);
	}

	void Finailize()
	{
		PlatformContext::Finalize();
	}

	void RegisteryDeviceNotify(IDeviceNotify* deviceNotify)
	{
		PlatformContext::RegisterDeviceNotify(deviceNotify);
	}

	void WaitForGpu()
	{
		PlatformContext::WaitForGpu();
	}

	void SubmitFrame()
	{
		PlatformContext::SubmitFrame();
	}

}