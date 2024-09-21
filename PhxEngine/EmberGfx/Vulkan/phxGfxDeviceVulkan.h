#pragma once
#include "EmberGfx/phxGfxDeviceInterface.h"

namespace phx::gfx
{
	class GfxDeviceVulkan final : public IGfxDevice
	{
	public:
		GfxDeviceVulkan() = default;
		~GfxDeviceVulkan() = default;

		void Initialize() override;
	};
}

