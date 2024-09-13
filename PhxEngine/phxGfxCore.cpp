#include "pch.h"
#include "phxGfxCore.h"
#include "phxDisplay.h"
#include "phxGfxContext.h"

using namespace phx;

#pragma warning(disable : 4061)

namespace phx::gfx
{
	void Initialize()
	{
		PlatformContext::Initialize();

		// Create COmmand Handlers
		// Create Create Context manager
		Display::Initialize();

		// Initialize Other gfx Systems
	}

	void Finalize()
	{
		PlatformContext::Finalize();
	}
}
