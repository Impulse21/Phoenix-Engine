#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResource.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

void phx::renderer::TextureResource::Dispose()
{
	rhi::DeleteTexture(texture_handle);
}

bool phx::renderer::TextureResource::CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index)
{

	if (fill_index + 1 >= transitions.Size() || state != ResourceState::Pending_gfx_transition)
		return false;

	transitions[fill_index++] =
		rhi::GpuBarrier::CreateTexture(
			texture_handle,
			rhi::ResourceStates::CopyDest,
			rhi::ResourceStates::ShaderResource);

	return true;
}
