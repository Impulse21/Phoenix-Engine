#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class IGpuMemoryAllocator
	{
	public:
		inline static IGpuMemoryAllocator* Ptr = nullptr;

	public:
		virtual ~IGpuMemoryAllocator() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual Budget GetBudget() = 0;
	};
}