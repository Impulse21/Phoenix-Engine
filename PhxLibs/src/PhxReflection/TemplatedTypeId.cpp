#include <PhxReflection/PhxReflection_pch.h>

#include "TemplatedTypeId.h"

#include <atomic>

using namespace phx::reflection;

TemplateTypeId GenerateNewTypeId()
{
	// Reserve id 0 for invalid id's.
	static constinit std::atomic<TemplateTypeId> s_IdCounter = 1;
	return s_IdCounter.fetch_add(1, std::memory_order_relaxed);
}
