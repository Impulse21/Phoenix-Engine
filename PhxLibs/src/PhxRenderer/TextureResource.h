#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/GfxDevice.h>

namespace phx::renderer
{
	struct TextureResource : public Resource
	{
		PHX_DECLARE_RESOURCE(TextureResource);

		rhi::TextureHandle TextureHandle;

		~TextureResource() override
		{
			rhi::GetDevice().DeleteTexture(TextureHandle);
		}
	};
}