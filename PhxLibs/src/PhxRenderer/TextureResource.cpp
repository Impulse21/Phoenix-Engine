#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResource.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

phx::renderer::TextureHotData::~TextureHotData()
{
	rhi::DeleteTexture(texture_handle);
}

bool phx::texture_ops::CollectPendingGpuTransitions(TextureResourceHandle texture_handle, SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	renderer::TextureHotData* hot_data = ResourceStore<renderer::TextureResource>::GetHot(texture_handle);
	if (fill_index >= transitions.Size() || hot_data->state != ResourceState::On_Gpu)
		return false;

	transitions[fill_index++] =
		GpuTransitionWork::CreateTexture(
			texture_handle,
			rhi::ResourceStates::ShaderResource);

	return true;
}
