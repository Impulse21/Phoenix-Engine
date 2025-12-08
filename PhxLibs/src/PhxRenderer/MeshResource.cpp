#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

phx::renderer::MeshHotData::~MeshHotData()
{
	rhi::DeleteBuffer(packed_mesh_buffer);
}

bool phx::CollectPendingGpuTransitions(MeshResourceHandle mesh_handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	renderer::MeshHotData* hot_data = ResourceStore<renderer::MeshResource>::GetHot(mesh_handle);
	if (fill_index >= transitions.Size() || hot_data->state != ResourceState::On_Gpu)
		return false;

	transitions[fill_index++] =
		GpuTransitionWork::CreateBuffer(
			hot_data->packed_mesh_buffer,
			rhi::ResourceStates::IndexGpuBuffer | rhi::ResourceStates::ShaderResourceNonPixel);

	return true;
}
