#include "PhxRenderer_pch.h"
#include <PhxRenderer/MeshResource.h>

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

void phx::renderer::MeshResource::Dispose()
{
	rhi::DeleteBuffer(packed_mesh_buffer);
}

using namespace phx;

bool phx::renderer::MeshResource::CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index)
{
	if (fill_index + 1 >= transitions.Size() || state != ResourceState::Pending_gfx_transition)
		return false;

	transitions[fill_index++] =
		rhi::GpuBarrier::CreateBuffer(
			packed_mesh_buffer,
			rhi::ResourceStates::CopyDest,
			rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel);

	return true;
}
