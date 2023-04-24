#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/RenderPath3DDeferred.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Profiler.h>
#include <taskflow/taskflow.hpp>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>
#include <PhxEngine/Shaders/ShaderInterop.h>

#include "DrawQueue.h"
#include <PhxEngine/Renderer/RenderPath3DDeferred.h>
#include <imgui.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::Core;


PhxEngine::Renderer::RenderPath3DDeferred::RenderPath3DDeferred(
	RHI::IGraphicsDevice* gfxDevice,
	std::shared_ptr<Renderer::CommonPasses> commonPasses,
	std::shared_ptr<Graphics::ShaderFactory> shaderFactory,
	std::shared_ptr<Core::FrameProfiler> frameProfiler)
	: m_gfxDevice(gfxDevice)
	, m_commonPasses(commonPasses)
	, m_shaderFactory(shaderFactory)
	, m_frameProfiler(frameProfiler)
{
}

void PhxEngine::Renderer::RenderPath3DDeferred::Initialize(DirectX::XMFLOAT2 const& canvasSize)
{
	tf::Executor executor;
	tf::Taskflow taskflow;

	this->WindowResize(canvasSize);
	tf::Task shaderLoadTask = this->LoadShaders(taskflow);
	tf::Task createPipelineStates = this->LoadPipelineStates(taskflow);
	// tf::Task createCommandSignatures = this->CreateCommandSignatures(taskflow);
	
	shaderLoadTask.precede(createPipelineStates);  // A runs before B and C
	// createCommandSignatures.succeed(createPipelineStates);
	tf::Future loadFuture = executor.run(taskflow);


	// -- Create Constant Buffers ---
	{
		RHI::BufferDesc bufferDesc = {};
		bufferDesc.Usage = RHI::Usage::Default;
		bufferDesc.Binding = RHI::BindingFlags::ConstantBuffer;
		bufferDesc.SizeInBytes = sizeof(Shader::New::Frame);
		bufferDesc.InitialState = RHI::ResourceStates::ConstantBuffer;

		bufferDesc.DebugName = "Frame Constant Buffer";
		this->m_frameCB = RHI::IGraphicsDevice::GPtr->CreateBuffer(bufferDesc);
	}

	this->m_clusterLighting.Initialize(this->m_gfxDevice, canvasSize);
	this->m_shadowAtlas.Initialize(this->m_gfxDevice);
	loadFuture.wait(); 
	this->CreateCommandSignatures(taskflow);

	// Create Command Signatre
}

void PhxEngine::Renderer::RenderPath3DDeferred::Render(Scene::Scene& scene, Scene::CameraComponent& mainCamera)
{
	Shader::New::Camera cameraData = {};
	cameraData.Proj = mainCamera.Projection;
	cameraData.View = mainCamera.View;
	cameraData.ViewProjection = mainCamera.ViewProjection;
	cameraData.ViewProjectionInv = mainCamera.ViewProjectionInv;
	cameraData.ProjInv = mainCamera.ProjectionInv;
	cameraData.ViewInv = mainCamera.ViewInv;
	std::memcpy(&cameraData.PlanesWS, &mainCamera.FrustumWS.Planes, sizeof(DirectX::XMFLOAT4) * 6);

	ICommandList* commandList = this->m_gfxDevice->BeginCommandRecording();


	// TODO: Disabling for now as this work is incomplete
#if false
	if (!this->m_depthPyramid.IsValid())
	{
		
		auto _ = commandList->BeginScopedMarker("Recreating depth PyramidPass");
		DirectX::XMUINT2 texSize = CreateDepthPyramid();
		/** these should have a memset to 0 just in case it's not keeping this code around
		RHI::SubresourceData subResourceData = {};
		subResourceData.rowPitch = uint64_t(texSize.x) * 2u; // 2 bytes per pixel 
		subResourceData.slicePitch = subResourceData.rowPitch * texSize.y;
		subResourceData.slicePitch = 0;

		const uint64_t blackImage = 0u;
		subResourceData.pData = &blackImage;
		upload->WriteTexture(this->BlackTexture, 0, 1, &subResourceData);
		*/
	}
#endif 
	{
		auto rangeId = this->m_frameProfiler->BeginRangeGPU("Prepare Render Data", commandList);
		this->PrepareFrameRenderData(commandList, mainCamera, scene);

		if (this->m_gfxDevice->CheckCapability(DeviceCapability::RayTracing))
		{
			// Disable Raytracing for now.
			// this->UpdateRTAccelerationStructures(commandList, scene);
		}
		this->m_frameProfiler->EndRangeGPU(rangeId);
	}

	RHI::Viewport v(this->m_canvasSize.x, this->m_canvasSize.y);
	RHI::Rect rec(LONG_MAX, LONG_MAX);
	auto instanceView = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent>();

	static Shader::New::Camera cullCamera = {};
	if (!this->m_settings.FreezeCamera)
	{
		cullCamera = cameraData;
	}

	if (scene.GetShaderData().LightCount > 0)
	{
		auto _ = commandList->BeginScopedMarker("Fill Per Lighting Buffer");
		RHI::GpuBarrier preBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetPerlightMeshInstances(), this->m_gfxDevice->GetBufferDesc(scene.GetPerlightMeshInstances()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetPerlightMeshInstancesCounts(), this->m_gfxDevice->GetBufferDesc(scene.GetPerlightMeshInstancesCounts()).InitialState, RHI::ResourceStates::UnorderedAccess),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

		commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::FillPerLightInstances]);

		Shader::New::FillPerLightListConstants push = {};
		push.UseMeshlets = this->m_settings.EnableMeshShaders;
		push.PerLightMeshCountsUavIdx = this->m_gfxDevice->GetDescriptorIndex(scene.GetPerlightMeshInstancesCounts(), RHI::SubresouceType::UAV);
		push.PerLightMeshInstancesUavIdx = this->m_gfxDevice->GetDescriptorIndex(scene.GetPerlightMeshInstances(), RHI::SubresouceType::UAV);

		commandList->BindPushConstant(0, push);
		commandList->BindConstantBuffer(1, this->m_frameCB);

		static Shader::New::Camera cullCamera = {};
		if (!this->m_settings.FreezeCamera)
		{
			cullCamera = cameraData;
		}

		commandList->BindDynamicConstantBuffer(2, cullCamera);
		const int numThreadGroups = (int)std::ceilf(scene.GetShaderData().InstanceCount * scene.GetShaderData().LightCount / (float)THREADS_PER_WAVE);
		commandList->Dispatch(
			numThreadGroups,
			1,
			1);

		RHI::GpuBarrier postBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetPerlightMeshInstances(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetPerlightMeshInstances()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetPerlightMeshInstancesCounts(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetPerlightMeshInstancesCounts()).InitialState),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));
	}

	if (scene.GetShaderData().LightCount > 0)
	{
		auto _ = commandList->BeginScopedMarker("Fill Shadow Indirect Draw Buffers");
		RHI::GpuBarrier preBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshletBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshletBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawPerLightCountBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawPerLightCountBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

		commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::FillLightDrawBuffers]);

		Shader::New::FillLightDrawBuffers push = {};
		push.UseMeshlets = this->m_settings.EnableMeshShaders;
		push.DrawBufferIdx = push.UseMeshlets
			? this->m_gfxDevice->GetDescriptorIndex(scene.GetIndirectDrawShadowMeshletBuffer(), RHI::SubresouceType::UAV)
			: this->m_gfxDevice->GetDescriptorIndex(scene.GetIndirectDrawShadowMeshBuffer(), RHI::SubresouceType::UAV);

		push.DrawBufferCounterIdx = this->m_gfxDevice->GetDescriptorIndex(scene.GetIndirectDrawPerLightCountBuffer(), RHI::SubresouceType::UAV);

		commandList->BindPushConstant(0, push);
		commandList->BindConstantBuffer(1, this->m_frameCB);

		static Shader::New::Camera cullCamera = {};
		if (!this->m_settings.FreezeCamera)
		{
			cullCamera = cameraData;
		}

		commandList->BindDynamicConstantBuffer(2, cullCamera);
		const int numThreadGroups = (int)std::ceilf(scene.GetShaderData().LightCount / (float)THREADS_PER_WAVE);
		commandList->Dispatch(
			numThreadGroups,
			1,
			1);

		RHI::GpuBarrier postBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshBuffer(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshBuffer()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshletBuffer(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshletBuffer()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawPerLightCountBuffer(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawPerLightCountBuffer()).InitialState),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));
	}

	{
		auto _ = commandList->BeginScopedMarker("Culling Pass (Early)");
		RHI::GpuBarrier preBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshletBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshletBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
			RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesCounterBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesCounterBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

		commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::CullPass]);

		Shader::New::CullPushConstants push = {};
		push.IsLatePass = false;
		push.DrawBufferMeshIdx = scene.GetShaderData().IndirectEarlyMeshBufferIdx;
		push.DrawBufferMeshletIdx = scene.GetShaderData().IndirectEarlyMeshletBufferIdx;
		push.DrawBufferTransparentMeshIdx = scene.GetShaderData().IndirectEarlyTransparentMeshBufferIdx;
		push.DrawBufferTransparentMeshletIdx = scene.GetShaderData().IndirectEarlyTransparentMeshletBufferIdx;
		push.CulledDataCounterSrcIdx = RHI::cInvalidDescriptorIndex;
		push.CulledDataSRVIdx = RHI::cInvalidDescriptorIndex;

		commandList->BindPushConstant(0, push);

		commandList->BindConstantBuffer(1, this->m_frameCB);

		static Shader::New::Camera cullCamera = {};
		if (!this->m_settings.FreezeCamera)
		{
			cullCamera = cameraData;
		}

		commandList->BindDynamicConstantBuffer(2, cullCamera);
		const int numThreadGroups = (int)std::ceilf(instanceView.size() / (float)THREADS_PER_WAVE);
		commandList->Dispatch(
			numThreadGroups,
			1,
			1);

		RHI::GpuBarrier postBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshBuffer(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshBuffer()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshletBuffer(),RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshletBuffer()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesBuffer(), RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesBuffer()).InitialState),
			RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesCounterBuffer(), RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesCounterBuffer()).InitialState),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));
	}

	if (this->m_shadowAtlas.ShadowRenderPass.IsValid() && !this->m_settings.EnableMeshShaders)
	{
		auto _ = commandList->BeginScopedMarker("Shadow Pass");

		commandList->BeginRenderPass(this->m_shadowAtlas.ShadowRenderPass);
		if (this->m_settings.EnableMeshShaders)
		{
			commandList->SetMeshPipeline(this->m_meshStates[EMeshPipelineStates::ShadowPass]);
		}
		else
		{
			commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::ShadowPass]);
			commandList->BindIndexBuffer(scene.GetGlobalIndexBuffer());
		}

		Shader::New::GeometryPushConstant push = {};
		push.DrawId = -1;
		commandList->BindPushConstant(0, push);

		commandList->BindConstantBuffer(1, this->m_frameCB);
		commandList->BindDynamicConstantBuffer(2, cameraData);
		auto lightView = scene.GetAllEntitiesWith<Scene::LightComponent, Scene::NameComponent>();
		for (auto e : lightView)
		{
			auto [light, name] = lightView.get<Scene::LightComponent, Scene::NameComponent>(e);
			if (!light.IsEnabled() || !light.CastShadows())
			{
				continue;
			}

			switch (light.Type)
			{
			case Shader::New::LIGHT_TYPE_DIRECTIONAL:
			{
				std::array<Viewport, kNumCascades> cascadeViews;
				std::array<RHI::Rect, kNumCascades> scissors;
				for (int i = 0; i < cascadeViews.size(); i++)
				{
					Viewport& vp = cascadeViews[i];
					vp.MinX = float(light.ShadowRect.x + i * light.ShadowRect.w);
					vp.MaxX = float(light.ShadowRect.w + vp.MinX);
					vp.MinY = float(light.ShadowRect.y);
					vp.MaxY = float(light.ShadowRect.y + light.ShadowRect.h);
					vp.MinZ = 0.0f;
					vp.MaxZ = 1.0f;
					scissors[i] = rec;
				}
				commandList->SetViewports(cascadeViews.data(), cascadeViews.size());
				commandList->SetScissors(scissors.data(), scissors.size());
				std::array<ShadowCam, kNumCascades> shadowCams;
				Renderer::CreateDirectionLightShadowCams(
					mainCamera,
					light,
					shadowCams.data());

				Shader::New::ShadowCams shaderSCams;
				for (int i = 0; i < shadowCams.size(); i++)
				{
					DirectX::XMStoreFloat4x4(&shaderSCams.ViewProjection[i], shadowCams[i].ViewProjection);
				}
				commandList->BindDynamicConstantBuffer(3, shaderSCams);
				break;
			}
			case Shader::New::LIGHT_TYPE_OMNI:
			{
				std::array<Viewport, 6> cubeViews;
				std::array<RHI::Rect, 6> scissors;
				for (int i = 0; i < cubeViews.size(); i++)
				{
					Viewport& vp = cubeViews[i];
					vp.MinX = float(light.ShadowRect.x + i * light.ShadowRect.w);
					vp.MaxX = float(light.ShadowRect.w + vp.MinX);
					vp.MinY = float(light.ShadowRect.y);
					vp.MaxY = float(light.ShadowRect.y + light.ShadowRect.h);
					vp.MinZ = 0.0f;
					vp.MaxZ = 1.0f;
					scissors[i] = rec;
				}
				commandList->SetViewports(cubeViews.data(), cubeViews.size());
				commandList->SetScissors(scissors.data(), scissors.size());

				const float zNearP = 0.1f;
				const float zFarP = std::max(1.0f, light.GetRange());
				std::array<ShadowCam, 6> shadowCams;
				Renderer::CreateCubemapCameras(
					light.Position,
					zNearP,
					zFarP,
					shadowCams.data());

				Shader::New::ShadowCams shaderSCams;
				for (int i = 0; i < shadowCams.size(); i++)
				{
					DirectX::XMStoreFloat4x4(&shaderSCams.ViewProjection[i], shadowCams[i].ViewProjection);
				}
				commandList->BindDynamicConstantBuffer(3, shaderSCams);
				break;
			}

			case Shader::New::LIGHT_TYPE_SPOT:
			{
				Viewport vp;
				vp.MinX = float(light.ShadowRect.x);
				vp.MaxX = float(light.ShadowRect.w + light.ShadowRect.x);
				vp.MinY = float(light.ShadowRect.y);
				vp.MaxY = float(light.ShadowRect.y + light.ShadowRect.h);
				vp.MinZ = 0.0f;
				vp.MaxZ = 1.0f;
				commandList->SetViewports(&vp, 1);
				commandList->SetScissors(&rec, 1);
				ShadowCam shadowCam;
				Renderer::CreateSpotLightShadowCam(
					mainCamera,
					light,
					shadowCam);

				Shader::New::ShadowCams shaderSCams;
				DirectX::XMStoreFloat4x4(&shaderSCams.ViewProjection[0], shadowCam.ViewProjection);
				commandList->BindDynamicConstantBuffer(3, shaderSCams);
				break;
			}
			}

#if true
			auto nameScope = commandList->BeginScopedMarker(name.Name);
			commandList->ExecuteIndirect(
				this->m_settings.EnableMeshShaders ? this->m_commandSignatures[ECommandSignatures::Shadows_MS] : this->m_commandSignatures[ECommandSignatures::Shadows_Gfx],
				this->m_settings.EnableMeshShaders ? scene.GetIndirectDrawShadowMeshletBuffer() : scene.GetIndirectDrawShadowMeshBuffer(),
				(this->m_settings.EnableMeshShaders ? sizeof(Shader::New::MeshletDrawCommand) : sizeof(Shader::New::MeshDrawCommand)) * light.GlobalBufferIndex * scene.GetShaderData().InstanceCount,
				scene.GetIndirectDrawPerLightCountBuffer(),
				sizeof(uint32_t) * light.GlobalBufferIndex,
				scene.GetShaderData().InstanceCount);
#endif
		}

		commandList->EndRenderPass();
	}

	{
		auto _ = commandList->BeginScopedMarker("GBuffer Fill Pass (Early)");
		this->GBufferFillPass(
			commandList,
			scene,
			cameraData,
			&v, &rec,
			scene.GetIndirectDrawEarlyMeshBuffer(),
			scene.GetIndirectDrawEarlyMeshletBuffer());
	}

	// Disabling occlussion stuff for now. Will implement it at a later time.
#if false
	// Update depth Pyramid
	{
		auto _ = commandList->BeginScopedMarker("Generate Depth Pyramid Chain");
		commandList->SetComputeState(this->m_computeStates[PsoComputeType::PSO_GenerateMipMaps_TextureCubeArray]);

		TextureDesc textureDesc = this->m_gfxDevice->GetTextureDesc(this->m_depthPyramid);

		uint32_t width = textureDesc.Width;
		uint32_t height = textureDesc.Width;
		Shader::New::DepthPyrmidPushConstnats push = {};
		for (int i = 0; i < textureDesc.MipLevels - 1; i++)
		{
			GpuBarrier preBarriers[] =
			{
				GpuBarrier::CreateTexture(this->m_depthPyramid, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1),
			};

			commandList->TransitionBarriers(Core::Span<GpuBarrier>(preBarriers, ARRAYSIZE(preBarriers)));

			width = std::max(1u, width / 2);
			height = std::max(1u, height / 2);

			push.inputTextureIdx = this->m_gfxDevice->GetDescriptorIndex(this->m_depthPyramid, SubresouceType::UAV, i);
			push.outputTextureIdx = this->m_gfxDevice->GetDescriptorIndex(this->m_depthPyramid, SubresouceType::UAV, i + 1);
			commandList->BindPushConstant(0, push);

			commandList->Dispatch(
				std::max(1u, (textureDesc.Width + BLOCK_SIZE_X_DEPTH_PYRMAMID - 1) / BLOCK_SIZE_X_DEPTH_PYRMAMID),
				std::max(1u, (textureDesc.Height + BLOCK_SIZE_Y_DEPTH_PYRMAMID - 1) / BLOCK_SIZE_Y_DEPTH_PYRMAMID),
				1);

			// Cannot continue 
			GpuBarrier postBarriers[] =
			{
				GpuBarrier::CreateMemory(),
				GpuBarrier::CreateTexture(this->m_depthPyramid, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1),
			};

			commandList->TransitionBarriers(Core::Span<GpuBarrier>(postBarriers, ARRAYSIZE(postBarriers)));
		}
	}

	{
		auto _ = commandList->BeginScopedMarker("Culling Pass (Late)");
		RHI::GpuBarrier preBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawLateBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawLateBuffer()).InitialState, RHI::ResourceStates::UnorderedAccess),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

		commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::CullPass]);

		Shader::New::CullPushConstants push = {};
		push.IsLatePass = true;
		push.DrawBufferIdx = scene.GetShaderData().IndirectLateBufferIdx;
		push.CulledDataCounterSrcIdx = this->m_gfxDevice->GetDescriptorIndex(scene.GetCulledInstancesCounterBuffer(), SubresouceType::SRV);
		push.CulledDataSRVIdx = this->m_gfxDevice->GetDescriptorIndex(scene.GetCulledInstancesBuffer(), SubresouceType::SRV);

		commandList->BindPushConstant(0, push);
		commandList->BindConstantBuffer(1, this->m_frameCB);

		commandList->BindDynamicConstantBuffer(2, cullCamera);
		const int numThreadGroups = (int)std::ceilf(instanceView.size() / (float)THREADS_PER_WAVE);
		commandList->Dispatch(
			numThreadGroups,
			1,
			1);

		RHI::GpuBarrier postBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawLateBuffer(), RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawLateBuffer()).InitialState),
		};
		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));
	}

	{
		auto _ = commandList->BeginScopedMarker("GBuffer Fill Pass (Late)");
		this->GBufferFillPass(
			commandList,
			scene,
			cameraData,
			&v, &rec,
			scene.GetIndirectDrawLateBuffer());
	}

#endif

	if (this->m_settings.EnableComputeDeferredLighting)
	{
		if (this->m_settings.EnableClusterLightDebugView)
		{
			auto _ = commandList->BeginScopedMarker("Cluster Light Debug View (Compute)");

			RHI::GpuBarrier preBarriers[] =
			{
				RHI::GpuBarrier::CreateTexture(this->m_colourBuffer, this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer).InitialState, RHI::ResourceStates::UnorderedAccess),
			};
			commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

			commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::ClusterLightsDebugPass]);
			commandList->BindConstantBuffer(1, this->m_frameCB);
			commandList->BindDynamicConstantBuffer(2, cameraData);
			commandList->BindDynamicUavDescriptorTable(3, { this->m_colourBuffer });
			auto& outputDesc = this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer);

			commandList->Dispatch(
				outputDesc.Width / DEFERRED_BLOCK_SIZE_X,
				outputDesc.Height / DEFERRED_BLOCK_SIZE_Y,
				1);

			RHI::GpuBarrier postTransition[] =
			{
				RHI::GpuBarrier::CreateTexture(this->m_colourBuffer, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer).InitialState),
			};

			commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));

		}
		else
		{
			auto _ = commandList->BeginScopedMarker("Deferred Lighting Pass (Compute)");
			auto rangeId = this->m_frameProfiler->BeginRangeGPU("Deferred Lighting Pass (Compute)", commandList);

			RHI::GpuBarrier preBarriers[] =
			{
				RHI::GpuBarrier::CreateTexture(this->m_colourBuffer, this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer).InitialState, RHI::ResourceStates::UnorderedAccess),
			};
			commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

			commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::DeferredLightingPass]);

			commandList->BindConstantBuffer(1, this->m_frameCB);
			commandList->BindDynamicConstantBuffer(2, cameraData);
			commandList->BindDynamicDescriptorTable(
				3,
				{
					this->m_gbuffer.DepthTex,
					this->m_gbuffer.AlbedoTex,
					this->m_gbuffer.NormalTex,
					this->m_gbuffer.SurfaceTex,
					this->m_gbuffer.SpecularTex,
					this->m_gbuffer.EmissiveTex,
				});

			commandList->BindDynamicUavDescriptorTable(4, { this->m_colourBuffer });

			auto& outputDesc = this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer);

			Shader::DefferedLightingCSConstants push = {};
			push.DipatchGridDim =
			{
				outputDesc.Width / DEFERRED_BLOCK_SIZE_X,
				outputDesc.Height / DEFERRED_BLOCK_SIZE_Y,
			};
			push.MaxTileWidth = 16;

			commandList->BindPushConstant(0, push);

			commandList->Dispatch(
				push.DipatchGridDim.x,
				push.DipatchGridDim.y,
				1);

			RHI::GpuBarrier postTransition[] =
			{
				RHI::GpuBarrier::CreateTexture(this->m_colourBuffer, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer).InitialState),
			};

			commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));

			this->m_frameProfiler->EndRangeGPU(rangeId);
		}
	}
	else
	{
		if (this->m_settings.EnableClusterLightDebugView)
		{
			auto _ = commandList->BeginScopedMarker("Cluster Lighting Debug pass");
			commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::DeferredLightingPass]);
			commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::ClusterLightsDebugPass]);
			commandList->SetViewports(&v, 1);
			commandList->SetScissors(&rec, 1);
			commandList->BindConstantBuffer(0, this->m_frameCB);
			commandList->BindDynamicConstantBuffer(1, cameraData);
			commandList->Draw(3, 1, 0, 0);
			commandList->EndRenderPass();
		}
		else
		{
			auto _ = commandList->BeginScopedMarker("Deferred Lighting Pass");
			auto rangeId = this->m_frameProfiler->BeginRangeGPU("Deferred Lighting Pass", commandList);
			commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::DeferredLightingPass]);
			commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::DeferredLightingPass]);
			commandList->SetViewports(&v, 1);
			commandList->SetScissors(&rec, 1);
			commandList->BindConstantBuffer(0, this->m_frameCB);
			commandList->BindDynamicConstantBuffer(1, cameraData);
			commandList->BindDynamicDescriptorTable(
				2,
				{
					this->m_gbuffer.DepthTex,
					this->m_gbuffer.AlbedoTex,
					this->m_gbuffer.NormalTex,
					this->m_gbuffer.SurfaceTex,
					this->m_gbuffer.SpecularTex,
					this->m_gbuffer.EmissiveTex,
				});

			commandList->Draw(3, 1, 0, 0);
			commandList->EndRenderPass();
			this->m_frameProfiler->EndRangeGPU(rangeId);
		}
	}

	{
		auto __ = commandList->BeginScopedMarker("Post Process");
		{
			auto _ = commandList->BeginScopedMarker("Tone Mapping");
			auto rangeId = this->m_frameProfiler->BeginRangeGPU("Tone Mapping", commandList);
			commandList->BeginRenderPassBackBuffer();

			commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::ToneMappingPass]);
			commandList->SetViewports(&v, 1);
			commandList->SetScissors(&rec, 1);

			commandList->BindDynamicDescriptorTable(1, { this->m_colourBuffer });
			commandList->BindPushConstant(0, 1.0f);

			commandList->Draw(3);

			commandList->EndRenderPass();
			this->m_frameProfiler->EndRangeGPU(rangeId);
		}
	}

	commandList->Close();
	this->m_gfxDevice->ExecuteCommandLists({ commandList });
}

void PhxEngine::Renderer::RenderPath3DDeferred::WindowResize(DirectX::XMFLOAT2 const& canvasSize)
{
	this->m_canvasSize = canvasSize;
	{
		this->m_gbuffer.Free(this->m_gfxDevice);
		this->m_gbuffer.Initialize(this->m_gfxDevice, canvasSize);

		RHI::TextureDesc desc = {};
		desc.Width = std::max(canvasSize.x, 1.0f);
		desc.Height = std::max(canvasSize.y, 1.0f);
		desc.Dimension = RHI::TextureDimension::Texture2D;
		desc.IsBindless = true;
		desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
		desc.InitialState = RHI::ResourceStates::ShaderResource;
		desc.IsTypeless = true;
		desc.Format = RHI::RHIFormat::RGBA16_FLOAT;
		desc.DebugName = "Colour Buffer";
		desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::RenderTarget | RHI::BindingFlags::UnorderedAccess;

		if (this->m_colourBuffer.IsValid())
		{
			this->m_gfxDevice->DeleteTexture(this->m_colourBuffer);
		}
		this->m_colourBuffer = this->m_gfxDevice->CreateTexture(desc);
	}

	this->m_depthPyramidNumMips = 0;
	if (this->m_depthPyramid.IsValid())
	{
		this->m_gfxDevice->DeleteTexture(this->m_depthPyramid);
	}

	this->CreateRenderPasses();
}

void PhxEngine::Renderer::RenderPath3DDeferred::BuildUI()
{
	ImGui::Checkbox("Freeze Camera", &this->m_settings.FreezeCamera);
	ImGui::Checkbox("Enable Frustra Culling", &this->m_settings.EnableFrustraCulling);
	ImGui::Checkbox("Enable Occlusion Culling", &this->m_settings.EnableOcclusionCulling);
	ImGui::Checkbox("Enable Meshlets", &this->m_settings.EnableMeshShaders);
	if (this->m_settings.EnableMeshShaders)
	{
		ImGui::Checkbox("Enable Meshlet Culling", &this->m_settings.EnableMeshletCulling);
	}
	ImGui::Checkbox("Enable Compute Deferred Shading", &this->m_settings.EnableComputeDeferredLighting);
	ImGui::Checkbox("Enable Cluster Lighting", &this->m_settings.EnableClusterLightLighting);
	if (this->m_settings.EnableClusterLightLighting)
	{
		ImGui::Checkbox("View Cluster Light Heat Map", &this->m_settings.EnableClusterLightDebugView);
	}
}

tf::Task PhxEngine::Renderer::RenderPath3DDeferred::LoadShaders(tf::Taskflow& taskflow)
{
	tf::Task shaderLoadTask = taskflow.emplace([&](tf::Subflow& subflow) {
		// Load Shaders
		subflow.emplace([&]() {
			this->m_shaders[EShaders::VS_Rect] = this->m_shaderFactory->CreateShader("PhxEngine/RectVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "RectVS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::PS_Blit] = this->m_shaderFactory->CreateShader("PhxEngine/BlitPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "BlitPS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::VS_GBufferFill] = this->m_shaderFactory->CreateShader("PhxEngine/GBufferFillPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "GBufferFillPassVS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::PS_GBufferFill] = this->m_shaderFactory->CreateShader("PhxEngine/GBufferFillPassPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "GBufferFillPassPS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::VS_DeferredLightingPass] = this->m_shaderFactory->CreateShader("PhxEngine/DeferredLightingPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "DeferredLightingPassVS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::PS_DeferredLightingPass] = this->m_shaderFactory->CreateShader("PhxEngine/DeferredLightingPassPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "DeferredLightingPassPS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::CS_DeferredLightingPass] = this->m_shaderFactory->CreateShader("PhxEngine/DeferredLightingPassCS.hlsl", { .Stage = RHI::ShaderStage::Compute, .DebugName = "DeferredLightingPassCS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::PS_ToneMapping] = this->m_shaderFactory->CreateShader("PhxEngine/ToneMappingPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "ToneMappingPS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::CS_CullPass] = this->m_shaderFactory->CreateShader("PhxEngine/CullPassCS.hlsl", { .Stage = RHI::ShaderStage::Compute, .DebugName = "CullPassCS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::AS_MeshletCull] = this->m_shaderFactory->CreateShader("PhxEngine/MeshletCullAS.hlsl", { .Stage = RHI::ShaderStage::Mesh, .DebugName = "MeshletCullAS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::MS_MeshletGBufferFill] = this->m_shaderFactory->CreateShader("PhxEngine/GBufferFillMS.hlsl", { .Stage = RHI::ShaderStage::Mesh, .DebugName = "GBufferFillMS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::VS_ClusterLightsDebugPass] = this->m_shaderFactory->CreateShader("PhxEngine/ClusterLightingDebugPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "ClusterLightsDebugPassVS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::PS_ClusterLightsDebugPass] = this->m_shaderFactory->CreateShader("PhxEngine/ClusterLightingDebugPassPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "ClusterLightsDebugPassPS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::CS_ClusterLightsDebugPass] = this->m_shaderFactory->CreateShader("PhxEngine/ClusterLightingDebugPassCS.hlsl", { .Stage = RHI::ShaderStage::Compute, .DebugName = "ClusterLightsDebugPassCS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::CS_FillPerLightInstances] = this->m_shaderFactory->CreateShader("PhxEngine/FillPerLightInstancesCS.hlsl", { .Stage = RHI::ShaderStage::Compute, .DebugName = "FillPerLightInstancesCS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::CS_FillLightDrawBuffers] = this->m_shaderFactory->CreateShader("PhxEngine/FillLightDrawBuffersCS.hlsl", { .Stage = RHI::ShaderStage::Compute, .DebugName = "FillLightDrawBuffersCS", });
			});
		subflow.emplace([&]() {
			this->m_shaders[EShaders::VS_ShadowPass] = this->m_shaderFactory->CreateShader("PhxEngine/ShadowPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "ShadowPassVS", });
			});
		});
	return shaderLoadTask;
}

tf::Task PhxEngine::Renderer::RenderPath3DDeferred::LoadPipelineStates(tf::Taskflow& taskflow)
{
	tf::Task createPipelineStatesTask = taskflow.emplace([&](tf::Subflow& subflow) {
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::GBufferFillPass] = this->m_gfxDevice->CreateGraphicsPipeline({
					.VertexShader = this->m_shaders[EShaders::VS_GBufferFill],
					.PixelShader = this->m_shaders[EShaders::PS_GBufferFill],
					.RtvFormats = this->m_gbuffer.GBufferFormats,
					.DsvFormat = this->m_gbuffer.DepthFormat
				});
			});
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::ShadowPass] = this->m_gfxDevice->CreateGraphicsPipeline({
					.VertexShader = this->m_shaders[EShaders::VS_ShadowPass],
					.RasterRenderState = {.DepthClipEnable = true, .DepthBias = 1, .SlopeScaledDepthBias = 4},
					.DsvFormat = this->m_shadowAtlas.Format,
				});
			});
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::DeferredLightingPass] = this->m_gfxDevice->CreateGraphicsPipeline({
					.VertexShader = this->m_shaders[EShaders::VS_DeferredLightingPass],
					.PixelShader = this->m_shaders[EShaders::PS_DeferredLightingPass],
					.DepthStencilRenderState = {
						.DepthTestEnable = false,
					},
					.RtvFormats = { IGraphicsDevice::GPtr->GetTextureDesc(this->m_colourBuffer).Format },
				});
			});
		subflow.emplace([&]() {
			this->m_computeStates[EComputePipelineStates::DeferredLightingPass] = this->m_gfxDevice->CreateComputePipeline({
					.ComputeShader = this->m_shaders[EShaders::CS_DeferredLightingPass],
				});
			});
		subflow.emplace([&]() {
			this->m_computeStates[EComputePipelineStates::CullPass] = this->m_gfxDevice->CreateComputePipeline({
					.ComputeShader = this->m_shaders[EShaders::CS_CullPass],
				});
			});
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::ToneMappingPass] = this->m_gfxDevice->CreateGraphicsPipeline(
				{
					.PrimType = RHI::PrimitiveType::TriangleStrip,
					.VertexShader = this->m_shaders[EShaders::VS_Rect],
					.PixelShader = this->m_shaders[EShaders::PS_ToneMapping],
					.DepthStencilRenderState = {.DepthTestEnable = false, .StencilEnable = false },
					.RasterRenderState = {.CullMode = RHI::RasterCullMode::None },
					.RtvFormats = { this->m_gfxDevice->GetTextureDesc(this->m_gfxDevice->GetBackBuffer()).Format }
				});
			});
		subflow.emplace([&]() {
			this->m_meshStates[EMeshPipelineStates::GBufferFillPass] = this->m_gfxDevice->CreateMeshPipeline(
				{
					.AmpShader = this->m_shaders[EShaders::AS_MeshletCull],
					.MeshShader = this->m_shaders[EShaders::MS_MeshletGBufferFill],
					.PixelShader = this->m_shaders[EShaders::PS_GBufferFill],
					.RtvFormats = this->m_gbuffer.GBufferFormats,
					.DsvFormat = this->m_gbuffer.DepthFormat
				});
			});
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::ClusterLightsDebugPass] = this->m_gfxDevice->CreateGraphicsPipeline({
					.VertexShader = this->m_shaders[EShaders::VS_ClusterLightsDebugPass],
					.PixelShader = this->m_shaders[EShaders::PS_ClusterLightsDebugPass],
					.DepthStencilRenderState = {
						.DepthTestEnable = false,
					},
					.RtvFormats = { IGraphicsDevice::GPtr->GetTextureDesc(this->m_colourBuffer).Format },
				});
			});
		subflow.emplace([&]() {
			this->m_computeStates[EComputePipelineStates::ClusterLightsDebugPass] = this->m_gfxDevice->CreateComputePipeline({
					.ComputeShader = this->m_shaders[EShaders::CS_ClusterLightsDebugPass],
				});
			});
		subflow.emplace([&]() {
			this->m_computeStates[EComputePipelineStates::FillPerLightInstances] = this->m_gfxDevice->CreateComputePipeline({
					.ComputeShader = this->m_shaders[EShaders::CS_FillPerLightInstances],
				});
			});
		subflow.emplace([&]() {
			this->m_computeStates[EComputePipelineStates::FillLightDrawBuffers] = this->m_gfxDevice->CreateComputePipeline({
					.ComputeShader = this->m_shaders[EShaders::CS_FillLightDrawBuffers],
				});
			});
		});

	return createPipelineStatesTask;
}

tf::Task PhxEngine::Renderer::RenderPath3DDeferred::CreateCommandSignatures(tf::Taskflow& taskFlow)
{
	this->m_commandSignatures[ECommandSignatures::GBufferFill_MS] = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshletDrawCommand>({
				   .ArgDesc =
					   {
						   {.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
						   {.Type = IndirectArgumentType::DispatchMesh }
					   },
				   .PipelineType = PipelineType::Mesh,
				   .MeshHandle = this->m_meshStates[EMeshPipelineStates::GBufferFillPass] });
	this->m_commandSignatures[ECommandSignatures::GBufferFill_Gfx] = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshDrawCommand>({
				.ArgDesc =
					{
						{.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
						{.Type = IndirectArgumentType::DrawIndex }
					},
				.PipelineType = PipelineType::Gfx,
				.GfxHandle = this->m_gfxStates[EGfxPipelineStates::GBufferFillPass] });

	this->m_commandSignatures[ECommandSignatures::Shadows_Gfx] = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshDrawCommand>({
				.ArgDesc =
					{
						{.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
						{.Type = IndirectArgumentType::DrawIndex }
					},
				.PipelineType = PipelineType::Gfx,
				.GfxHandle = this->m_gfxStates[EGfxPipelineStates::ShadowPass] });

	/* TODO Mesh shader isn't ready yet
	subflow.emplace([&]() {
		this->m_commandSignatures[ECommandSignatures::Shadows_MS] = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshletDrawCommand>({
					   .ArgDesc =
						   {
							   {.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
							   {.Type = IndirectArgumentType::DispatchMesh }
						   },
					   .PipelineType = PipelineType::Mesh,
					   .MeshHandle = this->m_meshStates[EMeshPipelineStates::ShadowPass] });
		});
		*/

		tf::Task createCommandSignaturesTask = taskFlow.emplace([&](tf::Subflow& subflow) {});

	return createCommandSignaturesTask;
}

void PhxEngine::Renderer::RenderPath3DDeferred::CreateRenderPasses()
{
	if (this->m_renderPasses[ERenderPasses::GBufferFillPass].IsValid())
	{
		this->m_gfxDevice->DeleteRenderPass(this->m_renderPasses[ERenderPasses::GBufferFillPass]);
	}
	this->m_renderPasses[ERenderPasses::GBufferFillPass] = this->m_gfxDevice->CreateRenderPass(
		{
			.Attachments =
			{
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.AlbedoTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.NormalTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.SurfaceTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.SpecularTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.EmissiveTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
				{
					.Type = RenderPassAttachment::Type::DepthStencil,
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_gbuffer.DepthTex,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::DepthWrite,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
			}
		});

	if (this->m_renderPasses[ERenderPasses::DeferredLightingPass].IsValid())
	{
		this->m_gfxDevice->DeleteRenderPass(this->m_renderPasses[ERenderPasses::DeferredLightingPass]);
	}
	this->m_renderPasses[ERenderPasses::DeferredLightingPass] = this->m_gfxDevice->CreateRenderPass(
		{
			.Attachments =
			{
				{
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_colourBuffer,
					.InitialLayout = RHI::ResourceStates::ShaderResource,
					.SubpassLayout = RHI::ResourceStates::RenderTarget,
					.FinalLayout = RHI::ResourceStates::ShaderResource
				},
			}
		});
}

void PhxEngine::Renderer::RenderPath3DDeferred::PrepareFrameRenderData(
	ICommandList* commandList,
	Scene::CameraComponent const& mainCamera,
	Scene::Scene& scene)
{
	auto _ = commandList->BeginScopedMarker("Prepare Frame Data");

	Shader::New::Frame frameData = {};
	if (!this->m_settings.EnableFrustraCulling)
	{
		frameData.Flags |= Shader::New::FRAME_FLAGS_DISABLE_CULL_FRUSTUM;
	}
	if (!this->m_settings.EnableOcclusionCulling)
	{
		frameData.Flags |= Shader::New::FRAME_FLAGS_DISABLE_CULL_OCCLUSION;
	}
	if (!this->m_settings.EnableMeshletCulling)
	{
		frameData.Flags |= Shader::New::FRAME_FLAGS_DISABLE_CULL_MESHLET;
	}
	if (this->m_settings.EnableClusterLightLighting)
	{
		frameData.Flags |= Shader::New::FRAME_FLAGS_ENABLE_CLUSTER_LIGHTING;
	}
	
	frameData.SortedLightBufferIndex = this->m_gfxDevice->GetDescriptorIndex(this->m_clusterLighting.SortedLightBuffer, SubresouceType::SRV);
	frameData.LightLutBufferIndex = this->m_gfxDevice->GetDescriptorIndex(this->m_clusterLighting.LightLutBuffer, SubresouceType::SRV);
	frameData.LightTilesBufferIndex = this->m_gfxDevice->GetDescriptorIndex(this->m_clusterLighting.LightTilesBuffer, SubresouceType::SRV);

	frameData.SceneData = scene.GetShaderData();
	frameData.DepthPyramidIndex = this->m_gfxDevice->GetDescriptorIndex(this->m_depthPyramid, SubresouceType::SRV);

	// Update light Info 
	RHI::GPUAllocation lightAllocation;
	RHI::GPUAllocation lightMatricesAllocation;
	this->PrepareFrameLightData(commandList, mainCamera, scene, lightAllocation, lightMatricesAllocation);

	frameData.LightBufferIdx = this->m_gfxDevice->GetDescriptorIndex(this->m_lightBuffer, SubresouceType::SRV);
	frameData.LightMatrixBufferIdx = this->m_gfxDevice->GetDescriptorIndex(this->m_lightMatrixBuffer, SubresouceType::SRV);
	frameData.ShadowAtlasIdx = this->m_gfxDevice->GetDescriptorIndex(this->m_shadowAtlas.Texture, SubresouceType::SRV);
	frameData.ShadowAtlasRes.x = this->m_shadowAtlas.Width;
	frameData.ShadowAtlasRes.y = this->m_shadowAtlas.Height;
	frameData.ShadowAtlasResRCP.x = 1.0f / (float)this->m_shadowAtlas.Width;
	frameData.ShadowAtlasResRCP.y = 1.0f / (float)this->m_shadowAtlas.Height;
	frameData.SceneData = scene.GetShaderData();

	// Upload data
	RHI::GpuBarrier preCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_frameCB, RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshletBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshletBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawLateBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawLateBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshletBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshletBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesCounterBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesCounterBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_lightBuffer, this->m_gfxDevice->GetBufferDesc(this->m_lightBuffer).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_lightMatrixBuffer, this->m_gfxDevice->GetBufferDesc(this->m_lightMatrixBuffer).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), this->m_gfxDevice->GetBufferDesc(scene.GetInstanceBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.SortedLightBuffer, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.SortedLightBuffer).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.LightLutBuffer, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.LightLutBuffer).InitialState, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.LightTilesBuffer, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.LightTilesBuffer).InitialState, RHI::ResourceStates::CopyDest),
	};
	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

	commandList->WriteBuffer(this->m_frameCB, frameData);
	// TODO: Movies these to within the shaders that right them.
	commandList->WriteBuffer<uint32_t>(scene.GetIndirectDrawEarlyMeshBuffer(), 0, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshBuffer()).SizeInBytes - sizeof(uint32_t));
	commandList->WriteBuffer<uint32_t>(scene.GetIndirectDrawEarlyMeshletBuffer(), 0, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshletBuffer()).SizeInBytes - sizeof(uint32_t));
	commandList->WriteBuffer<uint32_t>(scene.GetIndirectDrawLateBuffer(), 0, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawLateBuffer()).SizeInBytes - sizeof(uint32_t));
	commandList->WriteBuffer<uint32_t>(scene.GetCulledInstancesCounterBuffer(), 0);
	commandList->WriteBuffer<uint32_t>(scene.GetIndirectDrawShadowMeshBuffer(), 0, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshBuffer()).SizeInBytes - sizeof(uint32_t));
	commandList->WriteBuffer<uint32_t>(scene.GetIndirectDrawShadowMeshletBuffer(), 0, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshletBuffer()).SizeInBytes - sizeof(uint32_t));

	commandList->CopyBuffer(
		this->m_lightBuffer,
		0,
		lightAllocation.GpuBuffer,
		lightAllocation.Offset,
		lightAllocation.SizeInBytes);

	commandList->CopyBuffer(
		this->m_lightMatrixBuffer,
		0,
		lightMatricesAllocation.GpuBuffer,
		lightMatricesAllocation.Offset,
		lightMatricesAllocation.SizeInBytes);

	commandList->CopyBuffer(
		scene.GetInstanceBuffer(),
		0,
		scene.GetInstanceUploadBuffer(),
		0,
		this->m_gfxDevice->GetBufferDesc(scene.GetInstanceUploadBuffer()).SizeInBytes);

	if (this->m_settings.EnableClusterLightLighting)
	{
		this->m_clusterLighting.Update(this->m_gfxDevice, commandList, scene, mainCamera);
	}

	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_frameCB, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawEarlyMeshletBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyMeshletBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawLateBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawLateBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetCulledInstancesCounterBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetCulledInstancesCounterBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetIndirectDrawShadowMeshletBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawShadowMeshletBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(this->m_lightBuffer, RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(this->m_lightBuffer).InitialState),
		RHI::GpuBarrier::CreateBuffer(this->m_lightMatrixBuffer, RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(this->m_lightMatrixBuffer).InitialState),
		RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(scene.GetInstanceBuffer()).InitialState),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.SortedLightBuffer, RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.SortedLightBuffer).InitialState),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.LightLutBuffer, RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.LightLutBuffer).InitialState),
		RHI::GpuBarrier::CreateBuffer(this->m_clusterLighting.LightTilesBuffer, RHI::ResourceStates::CopyDest, this->m_gfxDevice->GetBufferDesc(this->m_clusterLighting.LightTilesBuffer).InitialState),
	};

	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
}

void PhxEngine::Renderer::RenderPath3DDeferred::PrepareFrameLightData(
	RHI::ICommandList* commandList,
	Scene::CameraComponent const& mainCamera,
	Scene::Scene& scene,
	RHI::GPUAllocation& outLights,
	RHI::GPUAllocation& outMatrices)
{
	if (!this->m_lightBuffer.IsValid())
	{
		if (this->m_lightBuffer.IsValid())
		{
			RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_lightBuffer);
		}
		this->m_lightBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
				.MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::Bindless,
				.Binding = BindingFlags::ShaderResource,
				.InitialState = ResourceStates::ShaderResource,
				.StrideInBytes = sizeof(Shader::New::Light),
				.SizeInBytes = sizeof(Shader::New::Light) * MAX_NUM_LIGHTS,
				.DebugName = "Light Buffer",
			});

	}

	if (!this->m_lightMatrixBuffer.IsValid())
	{
		if (this->m_lightMatrixBuffer.IsValid())
		{
			RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_lightMatrixBuffer);
		}
		this->m_lightMatrixBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
				.MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::Bindless,
				.Binding = BindingFlags::ShaderResource,
				.InitialState = ResourceStates::ShaderResource,
				.StrideInBytes = sizeof(DirectX::XMFLOAT4X4),
				.SizeInBytes = sizeof(DirectX::XMFLOAT4X4) * MAX_NUM_LIGHTS,
				.DebugName = "Light Matrix Buffer",
			});
	}

	this->m_shadowAtlas.UpdatePerFrameData(commandList, scene, mainCamera.Eye);

	// Upload GPU data
	const auto& lightBufferDesc = this->m_gfxDevice->GetBufferDesc(this->m_lightBuffer);
	outLights = commandList->AllocateGpu(lightBufferDesc.SizeInBytes, lightBufferDesc.StrideInBytes);
	Shader::New::Light* plightBufferData = (Shader::New::Light*)outLights.CpuData;


	const auto& matrixBufferDesc = this->m_gfxDevice->GetBufferDesc(this->m_lightMatrixBuffer);
	outMatrices = commandList->AllocateGpu(matrixBufferDesc.SizeInBytes, matrixBufferDesc.StrideInBytes);
	DirectX::XMMATRIX* pMatrixBufferData = (DirectX::XMMATRIX*)outMatrices.CpuData;

	uint32_t currLight = 0;
	uint32_t matrixCounter = 0;
	auto lightView = scene.GetAllEntitiesWith<Scene::LightComponent>();;
	const XMFLOAT2 atlasDimRcp  = XMFLOAT2(1.0f / float(this->m_shadowAtlas.Width), 1.0f / float(this->m_shadowAtlas.Height));
	for (auto entity : lightView)
	{
		auto& light = lightView.get<Scene::LightComponent>(entity);
		if (!light.IsEnabled())
		{
			continue;
		}

		Shader::New::Light* shaderData = plightBufferData + currLight;
		*shaderData = {};

		shaderData->SetType(light.Type);
		shaderData->Position = light.Position;
		shaderData->SetRange(light.GetRange());

		shaderData->SetColor({ light.Colour.x * light.Intensity, light.Colour.y * light.Intensity, light.Colour.z * light.Intensity, 1 });

		if (light.CastShadows())
		{
			shaderData->ShadowAtlasMulAdd.x = light.ShadowRect.w * atlasDimRcp.x;
			shaderData->ShadowAtlasMulAdd.y = light.ShadowRect.h * atlasDimRcp.y;
			shaderData->ShadowAtlasMulAdd.z = light.ShadowRect.x * atlasDimRcp.x;
			shaderData->ShadowAtlasMulAdd.w = light.ShadowRect.y * atlasDimRcp.y;
			shaderData->GlobalMatrixIndex = matrixCounter;
		}
		else
		{
			shaderData->GlobalMatrixIndex = ~0u;
		}

		switch (light.Type)
		{
		case Scene::LightComponent::kDirectionalLight:
		{
			shaderData->SetDirection(light.Direction);

			if (light.CastShadows())
			{
				std::array<ShadowCam, kNumCascades> shadowCams;
				Renderer::CreateDirectionLightShadowCams(
					mainCamera,
					light,
					shadowCams.data());

				// Set Cascade Matrices
				for (auto& shCam : shadowCams)
				{
					std::memcpy(pMatrixBufferData + matrixCounter++, &shCam.ViewProjection, sizeof(XMMATRIX));
				}
			}
			break;
		}

		case Scene::LightComponent::kSpotLight:
		{
			const float outerConeAngle = light.OuterConeAngle;
			const float innerConeAngle = std::min(light.InnerConeAngle, outerConeAngle);
			const float outerConeAngleCos = std::cos(outerConeAngle);
			const float innerConeAngleCos = std::cos(innerConeAngle);

			// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#inner-and-outer-cone-angles
			const float lightAngleScale = 1.0f / std::max(0.001f, innerConeAngleCos - outerConeAngleCos);
			const float lightAngleOffset = -outerConeAngleCos * lightAngleScale;

			shaderData->SetConeAngleCos(outerConeAngleCos);
			shaderData->SetAngleScale(lightAngleScale);
			shaderData->SetAngleOffset(lightAngleOffset);
			shaderData->SetDirection(light.Direction);

			if (light.CastShadows())
			{
				ShadowCam shCam = {};
				CreateSpotLightShadowCam(mainCamera, light, shCam);
				std::memcpy(pMatrixBufferData + matrixCounter++, &shCam.ViewProjection, sizeof(XMMATRIX));
			}
			break;
		}

		case Scene::LightComponent::kOmniLight:
		{
			if (light.CastShadows())
			{
				const float nearZ = 0.1f;
				const float farZ = std::max(1.0f, light.GetRange());
				const float fRange = farZ / (farZ - nearZ);
				const float cubemapDepthRemapNear = fRange;
				const float cubemapDepthRemapFar = -fRange * nearZ;
				shaderData->SetCubeNearZ(cubemapDepthRemapNear);
				shaderData->SetCubeFarZ(cubemapDepthRemapFar);
			}
		}
		default:
		{
			// No-op
		}
		}
		light.GlobalBufferIndex = currLight++;
	}
}

void PhxEngine::Renderer::RenderPath3DDeferred::UpdateRTAccelerationStructures(ICommandList* commandList, PhxEngine::Scene::Scene& scene)
{
	if (!scene.GetTlas().IsValid())
	{
		return;
	}
#ifdef false
	auto __ = commandList->BeginScopedMarker("Prepare Frame RT Structures");
	BufferHandle instanceBuffer = this->m_gfxDevice->GetRTAccelerationStructureDesc(scene.GetTlas()).TopLevel.InstanceBuffer;
	{
		auto _ = commandList->BeginScopedMarker("Copy TLAS buffer");

		BufferHandle currentTlasUploadBuffer = scene.GetTlasUploadBuffer();

		commandList->TransitionBarrier(instanceBuffer, ResourceStates::Common, ResourceStates::CopyDest);
		commandList->CopyBuffer(
			instanceBuffer,
			0,
			currentTlasUploadBuffer,
			0,
			this->m_gfxDevice->GetBufferDesc(instanceBuffer).SizeInBytes);

	}

	{
		auto _ = commandList->BeginScopedMarker("BLAS update");
		auto view = scene.GetAllEntitiesWith<Scene::MeshComponent>();
		for (auto e : view)
		{
			auto& meshComponent = view.get<Scene::MeshComponent>(e);

			if (meshComponent.Blas.IsValid())
			{
				switch (meshComponent.BlasState)
				{
				case Scene::MeshComponent::BLASState::Rebuild:
					commandList->RTBuildAccelerationStructure(meshComponent.Blas);
					break;
				case Scene::MeshComponent::BLASState::Refit:
					assert(false);
					break;
				case Scene::MeshComponent::BLASState::Complete:
				default:
					break;
				}

				meshComponent.BlasState = Scene::MeshComponent::BLASState::Complete;
			}
		}
	}

	// Move to Scene
	// TLAS:
	{
		auto _ = commandList->BeginScopedMarker("TLAS Update");
		BufferDesc instanceBufferDesc = this->m_gfxDevice->GetBufferDesc(instanceBuffer);

		GpuBarrier preBarriers[] =
		{
			GpuBarrier::CreateBuffer(instanceBuffer, ResourceStates::CopyDest, ResourceStates::AccelStructBuildInput),
		};

		commandList->TransitionBarriers(Core::Span(preBarriers, ARRAYSIZE(preBarriers)));

		commandList->RTBuildAccelerationStructure(scene.GetTlas());

		GpuBarrier postBarriers[] =
		{
			GpuBarrier::CreateBuffer(instanceBuffer, ResourceStates::AccelStructBuildInput, ResourceStates::Common),
			GpuBarrier::CreateMemory()
		};

		commandList->TransitionBarriers(Core::Span(postBarriers, ARRAYSIZE(postBarriers)));
	}
#endif
}


void PhxEngine::Renderer::RenderPath3DDeferred::GBufferFillPass(
	RHI::ICommandList* commandList,
	Scene::Scene& scene,
	Shader::New::Camera cameraData,
	RHI::Viewport* v,
	RHI::Rect* scissor,
	RHI::BufferHandle indirectDrawMeshBuffer,
	RHI::BufferHandle indirectDrawMeshletBuffer)
{
	commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::GBufferFillPass]);

	if (this->m_settings.EnableMeshShaders)
	{
		// Dispatch Mesh
		commandList->SetMeshPipeline(this->m_meshStates[EMeshPipelineStates::GBufferFillPass]);
	}
	else
	{

		commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::GBufferFillPass]);
		commandList->BindIndexBuffer(scene.GetGlobalIndexBuffer());
	}

	commandList->SetViewports(v, 1);
	commandList->SetScissors(scissor, 1);
	commandList->BindConstantBuffer(1, this->m_frameCB);
	commandList->BindDynamicConstantBuffer(2, cameraData);

	BufferHandle indirectBuffer = this->m_settings.EnableMeshShaders ? indirectDrawMeshletBuffer : indirectDrawMeshBuffer;

	auto instanceView = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent>();
	commandList->ExecuteIndirect(
		this->m_settings.EnableMeshShaders ? this->m_commandSignatures[ECommandSignatures::GBufferFill_MS] : this->m_commandSignatures[ECommandSignatures::GBufferFill_Gfx],
		indirectBuffer,
		0,
		indirectBuffer,
		this->m_gfxDevice->GetBufferDesc(indirectBuffer).SizeInBytes - sizeof(uint32_t),
		instanceView.size());

	commandList->EndRenderPass();
}

DirectX::XMUINT2 PhxEngine::Renderer::RenderPath3DDeferred::CreateDepthPyramid()
{
	// Construct Depth Pyramid
	this->m_depthPyramidNumMips = 0;
	const RHI::TextureDesc& depthBufferDesc = this->m_gfxDevice->GetTextureDesc(this->m_gbuffer.DepthTex);
	uint32_t width = depthBufferDesc.Width / 2;
	uint32_t height = depthBufferDesc.Height / 2;
	this->m_depthPyramidNumMips = 0;
	while (width > 1 || height > 1)
	{
		this->m_depthPyramidNumMips++;
		width /= 2;
		height /= 2;
	}
	this->m_depthPyramid = this->m_gfxDevice->CreateTexture({
			.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::UnorderedAccess,
			.Dimension = TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.Format = RHIFormat::R32_FLOAT,
			.IsBindless = true,
			.Width = depthBufferDesc.Width,
			.Height = depthBufferDesc.Height,
			.MipLevels = this->m_depthPyramidNumMips,
			.OptmizedClearValue = { .Colour = {0.0f, 0.0f, 0.0f, 0.0f} },
			.DebugName = "Depth Pyramid",
		});

	// Create UAV views
	for (uint16_t i = 0; i < this->m_depthPyramidNumMips; i++)
	{
		int subIndex = IGraphicsDevice::GPtr->CreateSubresource(
			this->m_depthPyramid,
			RHI::SubresouceType::SRV,
			0,
			1,
			i,
			1);
		assert(i == subIndex);

		subIndex = this->m_gfxDevice->CreateSubresource(
			this->m_depthPyramid,
			RHI::SubresouceType::UAV,
			0,
			1,
			i,
			1);
		assert(i == subIndex);
	}

	return { depthBufferDesc.Width, depthBufferDesc.Height };
}

void PhxEngine::Renderer::RenderPath3DDeferred::DispatchCull(Shader::New::CullPushConstants const& push)
{
}
