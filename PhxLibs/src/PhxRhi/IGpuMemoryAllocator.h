#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class IGpuMemoryAllocator
	{
	public:
		virtual ~IGpuMemoryAllocator() = default;

		virtual Budget GetBudget() = 0;
	};
}