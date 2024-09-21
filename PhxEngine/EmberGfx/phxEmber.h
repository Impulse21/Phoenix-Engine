#pragma once


#include "phxGfxDevice.h"

namespace phx::gfx
{
	void InitializeWindows();
	void Finalize();
	class Ember
	{
	public:
		inline static Ember* Ptr = nullptr;

		GfxDevice* GfxDevice = nullptr;
	};
}

