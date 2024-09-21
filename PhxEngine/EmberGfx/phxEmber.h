#pragma once


#include "phxGfxDevice.h"

namespace phx::gfx
{
	void InitializeWindows();
	void Shutdown();
	class Ember
	{
	public:
		inline static Ember* Ptr = nullptr;

		GfxDevice* GfxDevice = nullptr;
	};
}

