#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

namespace phx::gfx
{
	class GfxDeviceD3D12 final : public IGfxDevice
	{
	public:
		GfxDeviceD3D12() = default;
		~GfxDeviceD3D12() = default;

		void Initialize() override;
	};
}

