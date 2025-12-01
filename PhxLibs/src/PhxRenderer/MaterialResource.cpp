#include "PhxRenderer/PhxRenderer_pch.h"
#include "MaterialResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::MaterialResource::~MaterialResource() = default;

bool phx::renderer::MaterialResource::CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> /*transitions*/, size_t& /*fill_index*/)
{
	return false;
}
