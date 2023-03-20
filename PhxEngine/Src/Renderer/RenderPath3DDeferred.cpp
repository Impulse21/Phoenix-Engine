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

	shaderLoadTask.precede(createPipelineStates);  // A runs before B and C

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

	loadFuture.wait();

	// Create Command Signatre
}

void PhxEngine::Renderer::RenderPath3DDeferred::Render(Scene::Scene& scene, Scene::CameraComponent& mainCamera)
{
	Shader::New::Camera cameraData = {};
	cameraData.ViewProjection = mainCamera.ViewProjection;
	cameraData.ViewProjectionInv = mainCamera.ViewProjectionInv;
	cameraData.ProjInv = mainCamera.ProjectionInv;
	cameraData.ViewInv = mainCamera.ViewInv;

	ICommandList* commandList = this->m_gfxDevice->BeginCommandRecording();
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

	// TODO: Bind Indirect Buffers
	assert(false);
	auto _ = commandList->BeginScopedMarker("Cull Pass");
	{
		commandList->SetComputeState(this->m_computeStates[EComputePipelineStates::CullPass]);
		commandList->Dispatch(
			(int)std::ceilf(instanceView.size() / THREADS_PER_WAVE),
			1,
			1);
	}

	{
		auto _ = commandList->BeginScopedMarker("GBuffer Fill Pass");
		commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::GBufferFillPass]);
		if (this->m_settings.EnableMeshShaders)
		{
			if (!this->m_drawMeshCommandSignatureMS.IsValid())
			{
				this->m_drawMeshCommandSignatureMS = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshDrawCommand>({
							   .ArgDesc =
								   {
									   {.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
									   {.Type = IndirectArgumentType::DispatchMesh }
								   },
							   .PipelineType = PipelineType::Mesh,
							   .MeshHandle = this->m_meshStates[EMeshPipelineStates::GBufferFillPass]
					});
			}

			// Dispatch Mesh
			commandList->SetMeshPipeline(this->m_meshStates[EMeshPipelineStates::GBufferFillPass]);
		}
		else
		{
			if (!this->m_drawMeshCommandSignatureGfx.IsValid())
			{
				// Create Command Signature
				this->m_drawMeshCommandSignatureGfx = this->m_gfxDevice->CreateCommandSignature<Shader::New::MeshDrawCommand>({
								.ArgDesc =
									{
										{.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
										{.Type = IndirectArgumentType::DrawIndex }
									},
								.PipelineType = PipelineType::Gfx,
								.GfxHandle = this->m_gfxStates[EGfxPipelineStates::GBufferFillPass]
					});
			}

			commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::GBufferFillPass]);


			commandList->ExecuteIndirect(
				this->m_drawMeshCommandSignatureGfx,
				scene.GetIndirectDrawEarlyBuffer(),
				offsetof(Shader::New::MeshDrawCommand, IndirectMS),
				scene.GetIndirectDrawEarlyBuffer(),
				this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyBuffer()).SizeInBytes - sizeof(uint32_t),
				instanceView.size());
		}

		commandList->SetViewports(&v, 1);
		commandList->SetScissors(&rec, 1);
		commandList->BindConstantBuffer(1, this->m_frameCB);
		commandList->BindDynamicConstantBuffer(2, cameraData);

		commandList->ExecuteIndirect(
			this->m_settings.EnableMeshShaders ? this->m_drawMeshCommandSignatureMS : this->m_drawMeshCommandSignatureGfx,
			scene.GetIndirectDrawEarlyBuffer(),
			offsetof(Shader::New::MeshDrawCommand, IndirectMS),
			scene.GetIndirectDrawEarlyBuffer(),
			this->m_gfxDevice->GetBufferDesc(scene.GetIndirectDrawEarlyBuffer()).SizeInBytes - sizeof(uint32_t),
			instanceView.size());

		commandList->EndRenderPass();
	}

	if (this->m_settings.EnableComputeDeferredLighting)
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
			});

		commandList->Draw(3, 1, 0, 0);
		commandList->EndRenderPass();
		this->m_frameProfiler->EndRangeGPU(rangeId);
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
		desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::RenderTarget;

		if (this->m_colourBuffer.IsValid())
		{
			this->m_gfxDevice->DeleteTexture(this->m_colourBuffer);
		}
		this->m_colourBuffer = this->m_gfxDevice->CreateTexture(desc);
	}

	this->CreateRenderPasses();
}

void PhxEngine::Renderer::RenderPath3DDeferred::BuildUI()
{
	// TODO: Set up UI
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
			this->m_meshStates[EGfxPipelineStates::GBufferFillPass_Mesh] = this->m_gfxDevice->CreateMeshPipeline(
				{
					.AmpShader = this->m_shaders[EShaders::AS_MeshletCull],
					.MeshShader = this->m_shaders[EShaders::MS_MeshletGBufferFill],
					.PixelShader = this->m_shaders[EShaders::PS_GBufferFill],
					.RtvFormats = this->m_gbuffer.GBufferFormats,
					.DsvFormat = this->m_gbuffer.DepthFormat
				});
			});
		});

	return createPipelineStatesTask;
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
	frameData.SceneData = scene.GetShaderData();

	assert(false); // TODO: Do this else where
	// Upload data
	RHI::GpuBarrier preCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_frameCB, RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest),
	};
	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

	commandList->WriteBuffer(this->m_frameCB, frameData);
	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_frameCB, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer),
	};

	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
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
