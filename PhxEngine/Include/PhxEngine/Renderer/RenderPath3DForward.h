#pragma once

#include <array>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

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

	namespace Forward::EGfxPipelineStates
	{
		enum
		{
			DepthPass,
			GeometryPass,
			ToneMappingPass,
			Count
		};
	}

	namespace Forward::EComputePipelineStates
	{
		enum
		{
			Count
		};
	}

	namespace Forward::EMeshPipelineStates
	{
		enum
		{
			Count
		};
	}

	namespace Forward::EShaders
	{
		enum
		{
			VS_DepthOnly,
			VS_ForwardGeometry,
			VS_Rect,

			PS_ForwardGeometry,
			PS_Blit,
			PS_ToneMapping,

			Count
		};
	}

	namespace Forward::ERenderPasses
	{
		enum
		{
			DepthPass,
			GeometryShadePass,
			Count
		};
	}

	class RenderPath3DForward
	{
	public:
		RenderPath3DForward(
			RHI::IGraphicsDevice* gfxDevice,
			std::shared_ptr<Renderer::CommonPasses> commonPasses,
			std::shared_ptr<Graphics::ShaderFactory> shaderFactory,
			std::shared_ptr<Core::FrameProfiler> frameProfiler);

		void Initialize(DirectX::XMFLOAT2 const& canvasSize);

		void Render(Scene::Scene& scene, Scene::CameraComponent& cameraComponent);

		void WindowResize(DirectX::XMFLOAT2 const& canvasSize);

	private:
		tf::Task LoadShaders(tf::Taskflow& taskFlow);
		tf::Task LoadPipelineStates(tf::Taskflow& taskFlow);
		void CreateRenderPasses();

	private:
		void PrepareFrameRenderData(RHI::ICommandList* commandList, Scene::CameraComponent const& mainCamera, Scene::Scene& scen);
		void UpdateRTAccelerationStructures(RHI::ICommandList* commandList, Scene::Scene& scene);

		void RenderGeometry(RHI::ICommandList* commandList, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		std::shared_ptr<Core::FrameProfiler> m_frameProfiler;

		std::array<RHI::GraphicsPipelineHandle, Forward::EGfxPipelineStates::Count> m_gfxStates;
		std::array<RHI::ComputePipelineHandle, Forward::EComputePipelineStates::Count> m_computeStates;
		std::array<RHI::MeshPipelineHandle, Forward::EMeshPipelineStates::Count> m_meshStates;
		std::array<RHI::ShaderHandle, Forward::EShaders::Count> m_shaders;
		std::array<RHI::RenderPassHandle, Forward::ERenderPasses::Count> m_renderPasses;

		RHI::TextureHandle m_mainDepthTexture;
		RHI::TextureHandle m_colourBuffer;

		RHI::BufferHandle m_frameCB;

		DirectX::XMFLOAT2 m_canvasSize;
	};
}

