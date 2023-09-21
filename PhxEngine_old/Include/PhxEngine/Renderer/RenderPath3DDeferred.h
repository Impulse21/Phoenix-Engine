#pragma once

#include <array>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>
#include <PhxEngine/Renderer/GBuffer.h>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>
#include <PhxEngine/Renderer/ClusterLighting.h>
#include <PhxEngine/Renderer/Shadows.h>

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
	class DDGI;
	struct DrawQueue;

	namespace EGfxPipelineStates
	{
		enum
		{
			ShadowPass,
			GBufferFillPass,
			DeferredLightingPass,
			ClusterLightsDebugPass,
			ToneMappingPass,
			DDGI_DebugPass,
			Count
		};
	}

	namespace EComputePipelineStates
	{
		enum
		{
			CullPass,
			DepthPyramidGen,
			DeferredLightingPass,
			ClusterLightsDebugPass,
			FillPerLightInstances,
			FillLightDrawBuffers,
			DDGI_Raytrace,
			DDGI_UpdateProbeOffset,
			DDGI_UpdateVisibility,
			DDGI_UpdateIrradiance,
			DDGI_SampleIrradiance,
			Count
		};
	}

	namespace EMeshPipelineStates
	{
		enum
		{
			ShadowPass,
			GBufferFillPass,
			Count
		};
	}

	namespace EShaders
	{
		enum
		{
			VS_GBufferFill,
			VS_DeferredLightingPass,
			VS_ClusterLightsDebugPass,
			VS_ShadowPass,
			VS_Rect,
			VS_DDGI_Debug,

			PS_GBufferFill,
			PS_DeferredLightingPass,
			PS_ClusterLightsDebugPass,
			PS_Blit,
			PS_ToneMapping,
			PS_DDGI_Debug,

			CS_CullPass,
			CS_DepthPyramidGen,
			CS_DeferredLightingPass,
			CS_ClusterLightsDebugPass,
			CS_FillPerLightInstances,
			CS_FillLightDrawBuffers,
			CS_DDGI_RayTrace,
			CS_DDGI_UpdateProbeOffsets,
			CS_DDGI_UpdateIrradiance,
			CS_DDGI_UpdateVisibility,
			CS_DDGI_SampleIrradiance,

			AS_MeshletCull,
			AS_MeshletShadowCull,

			MS_MeshletGBufferFill,
			MS_MeshletShadowPass,

			Count
		};
	}

	namespace ERenderPasses
	{
		enum
		{
			GBufferFillPass,
			DeferredLightingPass,
			DebugPass,
			Count
		};
	}

	namespace ECommandSignatures
	{
		enum
		{
			GBufferFill_Gfx,
			GBufferFill_MS,

			Shadows_Gfx,
			Shadows_MS,

			Count,
		};
	}

	class RenderPath3DDeferred
	{
	public:
		RenderPath3DDeferred(
			RHI::IGraphicsDevice* gfxDevice,
			std::shared_ptr<Renderer::CommonPasses> commonPasses,
			std::shared_ptr<Graphics::ShaderFactory> shaderFactory,
			std::shared_ptr<Core::FrameProfiler> frameProfiler);

		void Initialize(DirectX::XMFLOAT2 const& canvasSize);

		void Render(Scene::Scene& scene, Scene::CameraComponent& cameraComponent);

		void WindowResize(DirectX::XMFLOAT2 const& canvasSize);

		void BuildUI();

		struct Settings
		{
			// TODO: Use a bit field
			bool EnableComputeDeferredLighting = false;
			bool EnableShadowPass = true;
			bool EnableGBufferMeshShaders = false;
			bool EnableShadowMeshShaders = false;
			bool EnableMeshletCulling = false;
			bool EnableFrustraCulling = true;
			bool EnableOcclusionCulling = true;
			bool FreezeCamera = false;
			bool EnableClusterLightLighting = false;
			bool EnableClusterLightDebugView = false;

			struct GI
			{
				bool EnableDDGI = false;
				bool DebugDrawProbes = false;
			} GISettings;
		};

		const Settings& GetSettings() { return this->m_settings; }

	private:
		tf::Task LoadShaders(tf::Taskflow& taskFlow);
		tf::Task LoadPipelineStates(tf::Taskflow& taskFlow);
		tf::Task CreateCommandSignatures(tf::Taskflow& taskFlow);
		void CreateRenderPasses();

	private:
		void PrepareFrameRenderData(RHI::ICommandList * commandList, Scene::CameraComponent const& mainCamera, Scene::Scene& scene);
		void PrepareFrameLightData(
			RHI::ICommandList* commandList,
			Scene::CameraComponent const& mainCamera,
			Scene::Scene& scene,
			RHI::GPUAllocation& outLights,
			RHI::GPUAllocation& outMatrices);
		void UploadRTData(RHI::ICommandList* commandList, Scene::Scene& scene);

		void RenderGeometry(RHI::ICommandList* commandList, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes);

		void GBufferFillPass(
			RHI::ICommandList* commandList,
			Scene::Scene& scene,
			Shader::New::Camera cameraData,
			RHI::Viewport* v,
			RHI::Rect* scissor,
			RHI::BufferHandle indirectDrawMeshBuffer,
			RHI::BufferHandle indirectDrawMeshletBuffer);

		DirectX::XMUINT2 CreateDepthPyramid();

		void DispatchCull(Shader::New::CullPushConstants const& push);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
		// std::unique_ptr<DDGI> m_ddgi;
		std::shared_ptr<Core::FrameProfiler> m_frameProfiler;

		std::array<RHI::GraphicsPipelineHandle, EGfxPipelineStates::Count> m_gfxStates;
		std::array<RHI::ComputePipelineHandle, EComputePipelineStates::Count> m_computeStates;
		std::array<RHI::MeshPipelineHandle, EMeshPipelineStates::Count> m_meshStates;
		std::array<RHI::ShaderHandle, EShaders::Count> m_shaders;
		std::array<RHI::RenderPassHandle, ERenderPasses::Count> m_renderPasses;
		std::array<RHI::CommandSignatureHandle, ECommandSignatures::Count> m_commandSignatures;

		ClusterLighting m_clusterLighting;

		GBufferRenderTargets m_gbuffer;
		RHI::TextureHandle m_colourBuffer;
		uint16_t m_depthPyramidNumMips = 1;
		RHI::TextureHandle m_depthPyramid; 
		RHI::BufferHandle m_frameCB;

		DirectX::XMFLOAT2 m_canvasSize;

		Renderer::ShadowsAtlas m_shadowAtlas;
		RHI::BufferHandle m_lightBuffer;
		RHI::BufferHandle m_lightMatrixBuffer;

		Settings m_settings;
		
	};
}
