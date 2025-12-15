#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResource.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxResource/ResourceManager.h>

phx::renderer::TextureResource::~TextureResource()
{
	rhi::DeleteTexture(texture_handle);
}

bool phx::texture_ops::CollectPendingGpuTransitions(GenericHandle generic_handle, SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index)
{
	Handle<renderer::TextureResource> texture_handle = generic_handle.To<renderer::TextureResource>();
	renderer::TextureResource* hot_data = ResourceStore<renderer::TextureResource>::GetHot(texture_handle);
	if (fill_index + 1 >= transitions.Size() || hot_data->state != ResourceState::Copied_to_gpu)
		return false;

	transitions[fill_index++] =
		rhi::GpuBarrier::CreateTexture(
			hot_data->texture_handle,
			rhi::ResourceStates::CopyDest,
			rhi::ResourceStates::ShaderResource);

	return true;
}
