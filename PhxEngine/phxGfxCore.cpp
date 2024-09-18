#include "pch.h"
#include "phxGfxCore.h"
#include "phxDisplay.h"

using namespace phx;

namespace phx::gfx
{
	void Initialize()
	{
		platform::Initialize();

		// TODO: Initialize Common Types
		Display::Initialize();

		// Add GPU Time Manager

		// Initialize Other gfx Systems
	}

	void Finalize()
	{
		platform::IdleGpu();
		platform::Finalize();
	}

}