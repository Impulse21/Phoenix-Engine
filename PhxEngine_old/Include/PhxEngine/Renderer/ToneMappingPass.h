#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <unordered_map>
#include <DirectXMath.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	class ToneMappingPass
	{
	public:
		ToneMappingPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<Renderer::CommonPasses> commonPasses);

		void Initialize(Graphics::ShaderFactory& factory);

		void OnWindowResize();

		void Render(RHI::ICommandList* commandList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle descRenderPass, DirectX::XMFLOAT2 const& canvas);

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
		std::unordered_map<PsoCacheKey, RHI::GraphicsPipelineHandle, PsoCacheKey::Hash> m_psoCache;

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		RHI::ShaderHandle m_pixelShader;

	};
}

