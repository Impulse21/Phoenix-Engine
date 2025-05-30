#pragma once

#include "RHICommon.h"
#include "RHIConfig.h"

namespace phx::rhi
{
	// Global accessor for the RHI Device instance
	inline GfxDevice& GetDevice()
	{
		static GfxDevice s_instance;
		return s_instance;
	}
}