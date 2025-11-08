#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	bool Initialize(Descriptor const& descriptor, void* window_handle);
	bool Shutdown();
}
