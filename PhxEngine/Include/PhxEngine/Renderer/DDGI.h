#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>
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
	struct DDGI
	{
		uint32_t FrameIndex = 0;
		DirectX::XMUINT3 GridDimensions = { 32, 8, 32 };
		DirectX::XMFLOAT3 GridMin = { -1, -1, -1 };
		DirectX::XMFLOAT3 GridMax = { 1, 1, 1 };
		uint32_t IrradianceOctSize = Shader::New::DDGI_COLOUR_RESOLUTION;
		uint32_t DepthOctSize = Shader::New::DDGI_DEPTH_RESOLUTION;
		int32_t RayCount = 192;

		RHI::TextureHandle RTRadianceOutput;
		RHI::TextureHandle RTDirectionDepthOutput;

		// TODO: maybe make this match the number of inflight frames
		std::array<RHI::TextureHandle, 2> ProbeIrradianceAtlas;
		std::array<RHI::TextureHandle, 2> ProbeVisibilityAtlas;

		RHI::TextureHandle SampleProbeGrid;;
		RHI::BufferHandle ProbeOffsetBuffer;

		uint32_t GetProbeCount() const { return this->GridDimensions.x * this->GridDimensions.y * this->GridDimensions.z; }
		void UpdateResources(RHI::IGraphicsDevice* gfxDevice);

		void BuildUI();
	};

#if false
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
			DirectX::XMINT3 ProbeCount;
			float ProbeMaxDistance = 4.0f; 
		} m_probeGrid;

		RHI::ShaderHandle m_rayTraceComputeShader;
		RHI::ComputePipelineHandle m_rayTraceComputePipeline;

		RHI::TextureHandle m_depthTexture;
		RHI::TextureHandle m_radianceTexture;

		std::vector<RHI::TextureHandle> m_irradianceTextures;
		std::vector<RHI::TextureHandle> m_depthTextures;
		RHI::BufferHandle m_rayBuffer;
	};
#endif

}

