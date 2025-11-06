#pragma once

#include "IDevice.h"

namespace phx::rhi
{
	bool Initialize(Descriptor const& descriptor);
	bool Shutdown();
}
