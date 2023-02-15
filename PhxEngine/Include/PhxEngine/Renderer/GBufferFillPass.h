#pragma once

#include <PhxEngine/Renderer/GeometryPasses.h>
#include <PhxEngine/Renderer/GBuffer.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	class GBufferFillPass : public IGeometryPass
	{
	public:
		GBufferFillPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<CommonPasses> commonPasses);
		~GBufferFillPass() = default;

		void Initialize(Graphics::ShaderFactory& shaderFactory);

		void WindowResized();

		void BeginPass(RHI::ICommandList* commandList, RHI::BufferHandle frameCB, Shader::Camera cameraData, GBufferRenderTargets const& gbufferRenderTargets);

		void BindPushConstant(RHI::ICommandList* commandList, Shader::GeometryPassPushConstants const& pushData) ;

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<CommonPasses> m_commonPasses;

		RHI::RenderPassHandle m_renderPass;
		RHI::GraphicsPipelineHandle m_gfxPipeline;
		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
	};
}

