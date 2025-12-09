#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

phx::renderer::MeshResource::~MeshResource()
{
	rhi::DeleteBuffer(packed_mesh_buffer);
}

bool phx::CollectPendingGpuTransitions(MeshResourceHandle mesh_handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	auto* mesh_resource = ResourceStore<renderer::MeshResource>::GetHot(mesh_handle);
	if (fill_index >= transitions.Size() || mesh_resource->state != ResourceState::On_Gpu)
		return false;

	transitions[fill_index++] =
		GpuTransitionWork::CreateBuffer(
			mesh_resource->packed_mesh_buffer,
			rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel);

	return true;
}
