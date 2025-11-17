#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::MeshResource::~MeshResource()
{
	rhi::IResourceManager::Ptr->DeleteBuffer(packed_mesh_buffer);
}

bool phx::renderer::MeshResource::CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	if (fill_index >= transitions.Size() || state != State::On_Gpu)
		return false;

	transitions[fill_index++] =
		GpuTransitionWork::CreateBuffer(
			packed_mesh_buffer,
			rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel);

	return true;
}
