#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
namespace PhxEngine::Renderer
{
	
	class RenderTarget
	{
		RenderTarget(
			RHI::IGraphicsDevice* gfxDevice,
			Core::Span<RHI::Texture> textures,
			RHI::Texture depth);

		~RenderTarget() = default;


	private:
		std::vector<RHI::TextureHandle> m_textures;
		size_t m_numTextures;

		RHI::TextureHandle m_depth;

		std::array<RHI::RHIFormat, RHI::cMaxRenderTargets> m_formats;
		RHI::RenderPassHandle m_renderPass;
	};
}