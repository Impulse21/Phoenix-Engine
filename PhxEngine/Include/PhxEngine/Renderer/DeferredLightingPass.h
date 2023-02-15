#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Renderer/GBuffer.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	class DeferredLightingPass
	{
	public:
		DeferredLightingPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<Renderer::CommonPasses> commonPasses);

		void Initialize(Graphics::ShaderFactory& factory);

		struct Input
		{
			RHI::TextureHandle Depth;
			RHI::TextureHandle GBufferAlbedo;
			RHI::TextureHandle GBufferNormals;
			RHI::TextureHandle GBufferSurface;
			RHI::TextureHandle GBufferSpecular;

			RHI::TextureHandle OutputTexture;

			RHI::BufferHandle FrameBuffer;
			Shader::Camera* CameraData;

			void FillGBuffer(GBufferRenderTargets const& renderTargets)
			{
				this->Depth = renderTargets.DepthTex;
				this->GBufferAlbedo = renderTargets.AlbedoTex;
				this->GBufferNormals = renderTargets.NormalTex;
				this->GBufferSurface = renderTargets.SurfaceTex;
				this->GBufferSpecular = renderTargets.SpecularTex;
			}
		};

		void Render(RHI::ICommandList* commandList, Input const& input);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		RHI::ShaderHandle m_pixelShader;
		RHI::ShaderHandle m_computeShader;
		RHI::ComputePipelineHandle m_computePso;
	};
}
