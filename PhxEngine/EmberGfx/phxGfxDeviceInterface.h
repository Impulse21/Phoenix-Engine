#pragma once

#include "phxGfxResources.h"

namespace phx::gfx
{
	class IGfxDevice
	{
	public:
		virtual void Initialize() = 0;

		virtual ~IGfxDevice() = default;
	};
}