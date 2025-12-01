#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::TextureResource::~TextureResource()
{
	rhi::DeleteTexture(TextureHandle);
}

bool phx::renderer::TextureResource::CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index)
{
	return false;
}
