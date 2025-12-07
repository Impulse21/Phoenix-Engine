#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::TextureResource::~TextureResource()
{
	rhi::DeleteTexture(texture_handle);
}

bool phx::renderer::TextureResource::CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	if (fill_index >= transitions.Size() || state != State::On_Gpu)
		return false;

	transitions[fill_index++] =
		GpuTransitionWork::CreateTexture(
			texture_handle,
			rhi::ResourceStates::ShaderResource);

	return true;
}
