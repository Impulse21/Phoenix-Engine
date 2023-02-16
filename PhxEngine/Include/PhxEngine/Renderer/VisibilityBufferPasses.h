#pragma once

#include <PhxEngine/Renderer/GeometryPasses.h>
#include <PhxEngine/Renderer/RenderTarget.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	class VisibilityBufferFillPass : public IGeometryPass
	{
	public:
		VisibilityBufferFillPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<CommonPasses> commonPasses);
		~VisibilityBufferFillPass() = default;

		void Initialize(Graphics::ShaderFactory& shaderFactory);

		void WindowResized();

		void BeginPass(RHI::ICommandList* commandList, RHI::BufferHandle frameCB, Shader::Camera cameraData, RHI::RenderPassHandle renderPass);

		void BindPushConstant(RHI::ICommandList* commandList, Shader::GeometryPassPushConstants const& pushData);

	private:
		enum GfxPipelines
		{
			ePipelineOpaque = 0,
			eAlphaPipeline,

			NumPipelines,
		};

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<CommonPasses> m_commonPasses;

		RHI::RenderPassHandle m_renderPass;
		std::array<RHI::GraphicsPipelineHandle, GfxPipelines::NumPipelines> m_gfxPipelines;

		RHI::ShaderHandle m_vertexShader;
		RHI::ShaderHandle m_pixelShader;
	};

	class MeshletBufferFillPass
	{
	public:
		MeshletBufferFillPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<CommonPasses> commonPasses);
		~MeshletBufferFillPass() = default;

		void Initialize(Graphics::ShaderFactory& shaderFactory);

		void Dispatch(RHI::ICommandList* commandList, RHI::BufferHandle meshletBuffer, size_t numInstances);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<CommonPasses> m_commonPasses;

		RHI::ComputePipelineHandle m_computePipeline;
		RHI::ShaderHandle m_computeShader;
	};
}

