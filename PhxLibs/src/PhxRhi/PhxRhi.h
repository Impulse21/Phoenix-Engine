#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	bool Initialize(Descriptor const& descriptor);
	bool Shutdown();
}
