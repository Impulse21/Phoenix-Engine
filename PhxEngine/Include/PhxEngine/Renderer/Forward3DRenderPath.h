#pragma once

#include <array>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

namespace tf
{
	class Task;
	class Taskflow;
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
			DepthPass,
			GeometryPass,
			ToneMappingPass,
			Count
		};
	}

	namespace EComputePipelineStates
	{
		enum
		{
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
			VS_DepthOnly,
			VS_Geomtry,
			VS_Rect,

			PS_GeometryShading,
			PS_Blit,
			PS_ToneMapping,

			Count
		};
	}

	namespace ERenderPasses
	{
		enum
		{
			DepthPass,
			GeometryShadePass,
			Count
		};
	}

	class Forward3DRenderPath
	{
	public:
		Forward3DRenderPath(
			RHI::IGraphicsDevice* gfxDevice,
			std::shared_ptr<Renderer::CommonPasses> commonPasses,
			std::shared_ptr<Graphics::ShaderFactory> shaderFactory);

		void Initialize(DirectX::XMFLOAT2 const& canvasSize);

		void Render(Scene::Scene& scene, Scene::CameraComponent& cameraComponent);

		void WindowResize(DirectX::XMFLOAT2 const& canvasSize);

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

		std::array<RHI::GraphicsPipelineHandle, EGfxPipelineStates::Count> m_gfxStates;
		std::array<RHI::GraphicsPipelineHandle, EComputePipelineStates::Count> m_computeStates;
		std::array<RHI::GraphicsPipelineHandle, EMeshPipelineStates::Count> m_meshStates;
		std::array<RHI::ShaderHandle, EMeshPipelineStates::Count> m_shaders;
		std::array<RHI::RenderPassHandle, ERenderPasses::Count> m_renderPasses;

		RHI::TextureHandle m_mainDepthTexture;
		RHI::TextureHandle m_colourBuffer;

		RHI::BufferHandle m_frameCB;

		DirectX::XMFLOAT2 m_canvasSize;
	};
}

