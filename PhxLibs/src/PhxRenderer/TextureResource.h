#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>

namespace phx::renderer
{
	struct TextureResource : public Resource
	{
		PHX_DECLARE_RESOURCE(TextureResource);

		RHI::TextureHandle TextureHandle;

		~TextureResource() override
		{
			RHI::DeleteTexture(TextureHandle);
		}
	};
}