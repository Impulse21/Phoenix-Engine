#pragma once

#include <array>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>
#include <PhxEngine/Renderer/GBuffer.h>

namespace tf
{
	class Task;
	class Taskflow;
}

namespace PhxEngine::Core
{
	class FrameProfiler;
}
namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Scene
{
	class Scene;
	struct CameraComponent;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	struct DrawQueue;

	namespace EGfxPipelineStates
	{
		enum
		{
			GBufferFillPass,
			DeferredLightingPass,
			ToneMappingPass,
			Count
		};
	}

	namespace EComputePipelineStates
	{
		enum
		{
			DeferredLightingPass,
			Count
		};
	}

	namespace EMeshPipelineStates
	{
		enum
		{
			Count
		};
	}

	namespace EShaders
	{
		enum
		{
			VS_GBufferFill,
			VS_DeferredLightingPass,
			VS_Rect,

			PS_GBufferFill,
			PS_DeferredLightingPass,
			PS_Blit,
			PS_ToneMapping,

			CS_DeferredLightingPass,

			Count
		};
	}

	namespace ERenderPasses
	{
		enum
		{
			GBufferFillPass,
			DeferredLightingPass,
			Count
		};
	}

	class Deferred3DRenderPath
	{
	public:
		Deferred3DRenderPath(
			RHI::IGraphicsDevice* gfxDevice,
			std::shared_ptr<Renderer::CommonPasses> commonPasses,
			std::shared_ptr<Graphics::ShaderFactory> shaderFactory,
			std::shared_ptr<Core::FrameProfiler> frameProfiler);

		void Initialize(DirectX::XMFLOAT2 const& canvasSize);

		void Render(Scene::Scene& scene, Scene::CameraComponent& cameraComponent);

		void WindowResize(DirectX::XMFLOAT2 const& canvasSize);

		void BuildUI();

	private:
		tf::Task LoadShaders(tf::Taskflow& taskFlow);
		tf::Task LoadPipelineStates(tf::Taskflow& taskFlow);
		void CreateRenderPasses();

	private:
		void PrepareFrameRenderData(RHI::ICommandList * commandList, Scene::CameraComponent const& mainCamera, Scene::Scene& scen);
		void UpdateRTAccelerationStructures(RHI::ICommandList* commandList, Scene::Scene& scene);

		void RenderGeometry(RHI::ICommandList* commandList, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		std::shared_ptr<Core::FrameProfiler> m_frameProfiler;

		std::array<RHI::GraphicsPipelineHandle, EGfxPipelineStates::Count> m_gfxStates;
		std::array<RHI::ComputePipelineHandle, EComputePipelineStates::Count> m_computeStates;
		std::array<RHI::MeshPipelineHandle, EMeshPipelineStates::Count> m_meshStates;
		std::array<RHI::ShaderHandle, EShaders::Count> m_shaders;
		std::array<RHI::RenderPassHandle, ERenderPasses::Count> m_renderPasses;

		GBufferRenderTargets m_gbuffer;
		RHI::TextureHandle m_colourBuffer;

		RHI::BufferHandle m_frameCB;

		DirectX::XMFLOAT2 m_canvasSize;

		struct Settings
		{
			bool EnableComputeDeferredLighting = false;
		} m_settings;
		
	};
}

