#pragma once

#include <atomic>

#include "ResourceTypes.h"

namespace phx
{
	struct Resource
	{
		std::atomic_uint8_t status = ResourceState::Unloaded;
		mutable std::atomic_uint32_t ref_count = 0;

		void AddRef() const { ref_count.fetch_add(1); }
		void Release() const;

		virtual void Dispose() = 0;

		virtual ~Resource() = default;
	};

}