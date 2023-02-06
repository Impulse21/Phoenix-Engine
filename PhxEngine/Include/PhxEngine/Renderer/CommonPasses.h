#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <unordered_map>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	class CommonPasses
	{
	public:
		CommonPasses(RHI::IGraphicsDevice* gfxDevice, Graphics::ShaderFactory& shaderFactory);

		void BlitTexture(RHI::ICommandList* cmdList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle descRenderPass, DirectX::XMFLOAT2 const& canvas);

	public:
		RHI::ShaderHandle FullScreenVS;
		RHI::ShaderHandle RectVS;
		RHI::ShaderHandle BlitPS;

		RHI::TextureHandle BlackTexture;
		RHI::TextureHandle WhiteTexture;

	protected:
		struct PsoCacheKey
		{
			std::vector<RHI::RHIFormat> RTVFormats;
			RHI::RHIFormat DepthFormat;

			bool operator==(const PsoCacheKey& other) const { return RTVFormats == other.RTVFormats && DepthFormat == other.DepthFormat; }
			bool operator!=(const PsoCacheKey& other) const { return !(*this == other); }

			struct Hash
			{
				size_t operator ()(const PsoCacheKey& s) const
				{
					size_t hash = 0;
					for (auto& format : s.RTVFormats)
					{
						RHI::HashCombine(hash, format);
					}

					RHI::HashCombine(hash, s.DepthFormat);
					return hash;
				}
			};
		};
	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		RHI::GraphicsPipelineHandle m_blitPipeline;
		std::unordered_map<PsoCacheKey, RHI::GraphicsPipelineHandle, PsoCacheKey::Hash> m_psoCache;
	};
}
