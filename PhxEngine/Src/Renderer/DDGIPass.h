#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Scene
{
	class Scene;
}

namespace tf
{
	class Task;
	class Taskflow;
}

namespace PhxEngine::Renderer
{
	class DDGI
	{
	public:
		void Initialize(tf::Taskflow& taskflow, Graphics::ShaderFactory& shaderFactory);

		void Render(RHI::ICommandList* commandList, Scene::Scene& scene);

		void BuidUI();

		struct Settings
		{
			bool OverrideProbeGridDimensions = false;
		};

		Settings& GetSettings() { return this->m_settings; }

	private:
		void InitializeProbeGrid(Scene::Scene const& scene);
		void RecreateProbeGrideResources();

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		DirectX::XMFLOAT2 m_canvas;
		Settings m_settings;
		uint32_t m_sceneId;

		struct ProbeGrid
		{
			float ProbeDistance = 1.0f;
			uint32_t RaysPerProbe = 256;
			DirectX::XMFLOAT3 GridStartPosition;
			DirectX::XMINT3 ProbeCount;
			float ProbeMaxDistance = 4.0f; 
			uint32_t IrradianceOctSize = 8;
			uint32_t DepthOctSize = 16;
		} m_probeGrid;

		RHI::ShaderHandle m_rayTraceComputeShader;
		RHI::ComputePipelineHandle m_rayTraceComputePipeline;

		RHI::TextureHandle m_depthTexture;
		RHI::TextureHandle m_radianceTexture;

		std::vector<RHI::TextureHandle> m_irradianceTextures;
		std::vector<RHI::TextureHandle> m_depthTextures;
		RHI::BufferHandle m_rayBuffer;
	};
}

