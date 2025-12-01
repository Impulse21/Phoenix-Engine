#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>

namespace phx::renderer
{
	struct TextureResource : public Resource
	{
		PHX_DECLARE_RESOURCE(TextureResource);

		rhi::TextureHandle TextureHandle;

		~TextureResource() override;

		bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) override;
	};
}