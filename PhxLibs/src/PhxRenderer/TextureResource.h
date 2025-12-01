#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>

namespace phx::renderer
{
	struct TextureResource : public Resource
	{
		PHX_DECLARE_RESOURCE(TextureResource);

		rhi::TextureHandle TextureHandle;

		~TextureResource() override
		{
			rhi::DeleteTexture(TextureHandle);
		}
	};
}