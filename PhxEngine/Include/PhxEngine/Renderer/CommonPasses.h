#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <unordered_map>

namespace PhxEngine::Renderer
{
	class CommonPasses
	{
	public:
		CommonPasses(RHI::IGraphicsDevice* gfxDevice, Graphics::ShaderFactory& shaderFactory);

		void BlitTexture(RHI::ICommandList* cmdList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle descRenderPass);

	public:
		RHI::ShaderHandle FullScreenVS;
		RHI::ShaderHandle RectVS;
		RHI::ShaderHandle BlitPS;

		RHI::TextureHandle BlackTexture;
		RHI::TextureHandle WhiteTexture;

	private:
		struct PsoCacheKey
		{
			RHI::RHIFormat Format;


			bool operator==(const PsoCacheKey& other) const { return Format == other.Format; }
			bool operator!=(const PsoCacheKey& other) const { return !(*this == other); }

			struct Hash
			{
				size_t operator ()(const PsoCacheKey& s) const
				{
					size_t hash = 0;
					RHI::HashCombine(hash, s.Format);
					return hash;
				}
			};
		};
	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		// std::unordered_map<PsoCacheKey, RHI::GraphicsPipelineHandle> m_psoCache;
	};
}

