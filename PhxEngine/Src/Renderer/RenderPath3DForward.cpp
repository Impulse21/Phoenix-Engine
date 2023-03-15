#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/RenderPath3DForward.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Profiler.h>
#include <taskflow/taskflow.hpp>

#include "DrawQueue.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::Renderer::Forward;
using namespace PhxEngine::Core;



PhxEngine::Renderer::RenderPath3DForward::RenderPath3DForward(
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

void PhxEngine::Renderer::RenderPath3DForward::Initialize(DirectX::XMFLOAT2 const& canvasSize)
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
		bufferDesc.SizeInBytes = sizeof(Shader::Frame);
		bufferDesc.InitialState = RHI::ResourceStates::ConstantBuffer;

		bufferDesc.DebugName = "Frame Constant Buffer";
		this->m_frameCB = RHI::IGraphicsDevice::GPtr->CreateBuffer(bufferDesc);
	}

	loadFuture.wait();
}

void PhxEngine::Renderer::RenderPath3DForward::Render(Scene::Scene& scene, Scene::CameraComponent& mainCamera)
{
	Shader::Camera cameraData = {};
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
			this->UpdateRTAccelerationStructures(commandList, scene);
		}
		this->m_frameProfiler->EndRangeGPU(rangeId);
	}

	RHI::Viewport v(this->m_canvasSize.x, this->m_canvasSize.y);
	RHI::Rect rec(LONG_MAX, LONG_MAX);

	// Fill Draw Queue
	static thread_local DrawQueue drawQueue;
	drawQueue.Reset();
	// Look through Meshes and instances?
	auto view = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::AABBComponent>();
	for (auto e : view)
	{
		auto [instanceComponent, aabbComp] = view.get<Scene::MeshInstanceComponent, Scene::AABBComponent>(e);

		auto& meshComponent = scene.GetRegistry().get<Scene::MeshComponent>(instanceComponent.Mesh);

		const float distance = Core::Math::Distance(mainCamera.Eye, aabbComp.BoundingData.GetCenter());

		drawQueue.Push((uint32_t)instanceComponent.Mesh, (uint32_t)e, distance);
	}

	drawQueue.SortOpaque();

	{
		auto rangeId = this->m_frameProfiler->BeginRangeGPU("Early Depth Pass", commandList);
		auto _ = commandList->BeginScopedMarker("Early Depth Pass");
		commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::DepthPass]);
		commandList->SetViewports(&v, 1);
		commandList->SetScissors(&rec, 1);
		commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::DepthPass]);
		commandList->BindConstantBuffer(1, this->m_frameCB);
		commandList->BindDynamicConstantBuffer(2, cameraData);
		this->RenderGeometry(commandList, scene, drawQueue, true);
		commandList->EndRenderPass();
		this->m_frameProfiler->EndRangeGPU(rangeId);
	}

	{
		auto _ = commandList->BeginScopedMarker("Geometry Shade");
		auto rangeId = this->m_frameProfiler->BeginRangeGPU("Geometry Shade", commandList);
		commandList->BeginRenderPass(this->m_renderPasses[ERenderPasses::GeometryShadePass]);
		commandList->SetGraphicsPipeline(this->m_gfxStates[EGfxPipelineStates::GeometryPass]);
		commandList->SetViewports(&v, 1);
		commandList->SetScissors(&rec, 1);
		commandList->BindConstantBuffer(1, this->m_frameCB);
		commandList->BindDynamicConstantBuffer(2, cameraData);
		this->RenderGeometry(commandList, scene, drawQueue, true);
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

void PhxEngine::Renderer::RenderPath3DForward::WindowResize(DirectX::XMFLOAT2 const& canvasSize)
{
	this->m_canvasSize = canvasSize;
	{
		RHI::TextureDesc desc = {};
		desc.Width = std::max(canvasSize.x, 1.0f);
		desc.Height = std::max(canvasSize.y, 1.0f);
		desc.Dimension = RHI::TextureDimension::Texture2D;
		desc.IsBindless = false;

		desc.Format = RHI::RHIFormat::D32;
		desc.IsTypeless = true;
		desc.DebugName = "Main Depth Buffer";
		desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
		desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
		desc.InitialState = RHI::ResourceStates::DepthRead;

		if (this->m_mainDepthTexture.IsValid())
		{
			this->m_gfxDevice->DeleteTexture(this->m_mainDepthTexture);
		}

		this->m_mainDepthTexture = this->m_gfxDevice->CreateTexture(desc);

		desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
		desc.InitialState = RHI::ResourceStates::ShaderResource;
		desc.IsBindless = true;
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

tf::Task PhxEngine::Renderer::RenderPath3DForward::LoadShaders(tf::Taskflow& taskflow)
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
		this->m_shaders[EShaders::VS_DepthOnly] = this->m_shaderFactory->CreateShader("PhxEngine/DepthPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "DepthPassVS", });
		});
	subflow.emplace([&]() {
		this->m_shaders[EShaders::VS_ForwardGeometry] = this->m_shaderFactory->CreateShader("PhxEngine/ForwardGeometryPassVS.hlsl", { .Stage = RHI::ShaderStage::Vertex, .DebugName = "ForwardGeometryPassVS", });
		});
	subflow.emplace([&]() {
		this->m_shaders[EShaders::PS_ForwardGeometry] = this->m_shaderFactory->CreateShader("PhxEngine/ForwardGeometryPassPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "ForwardGeometryPassPS", });
		});
	subflow.emplace([&]() {
		this->m_shaders[EShaders::PS_ToneMapping] = this->m_shaderFactory->CreateShader("PhxEngine/ToneMappingPS.hlsl", { .Stage = RHI::ShaderStage::Pixel, .DebugName = "ToneMappingPS", });
		});
		});
	return shaderLoadTask;
}

tf::Task PhxEngine::Renderer::RenderPath3DForward::LoadPipelineStates(tf::Taskflow& taskflow)
{
	tf::Task createPipelineStatesTask = taskflow.emplace([&](tf::Subflow& subflow) {
		subflow.emplace([&]() {
			this->m_gfxStates[EGfxPipelineStates::DepthPass] = this->m_gfxDevice->CreateGraphicsPipeline(
				{
					.VertexShader = this->m_shaders[EShaders::VS_DepthOnly],
					.DsvFormat = { this->m_gfxDevice->GetTextureDesc(this->m_mainDepthTexture).Format }
				});
			});
	subflow.emplace([&]() {
		this->m_gfxStates[EGfxPipelineStates::GeometryPass] = this->m_gfxDevice->CreateGraphicsPipeline(
			{
				.VertexShader = this->m_shaders[EShaders::VS_ForwardGeometry],
				.PixelShader = this->m_shaders[EShaders::PS_ForwardGeometry],
				.DepthStencilRenderState = {.DepthTestEnable = true, .DepthWriteEnable = false, .DepthFunc = ComparisonFunc::Equal },
				.RtvFormats = { this->m_gfxDevice->GetTextureDesc(this->m_colourBuffer).Format },
				.DsvFormat = { this->m_gfxDevice->GetTextureDesc(this->m_mainDepthTexture).Format },
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
		});

	return createPipelineStatesTask;
}

void PhxEngine::Renderer::RenderPath3DForward::CreateRenderPasses()
{
	if (this->m_renderPasses[ERenderPasses::DepthPass].IsValid())
	{
		this->m_gfxDevice->DeleteRenderPass(this->m_renderPasses[ERenderPasses::DepthPass]);
	}
	this->m_renderPasses[ERenderPasses::DepthPass] = this->m_gfxDevice->CreateRenderPass(
		{
			.Attachments =
			{
				{
					.Type = RenderPassAttachment::Type::DepthStencil,
					.LoadOp = RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_mainDepthTexture,
					.InitialLayout = RHI::ResourceStates::DepthRead,
					.SubpassLayout = RHI::ResourceStates::DepthWrite,
					.FinalLayout = RHI::ResourceStates::DepthRead
				},
			}
		});

	if (this->m_renderPasses[ERenderPasses::GeometryShadePass].IsValid())
	{
		this->m_gfxDevice->DeleteRenderPass(this->m_renderPasses[ERenderPasses::GeometryShadePass]);
	}
	this->m_renderPasses[ERenderPasses::GeometryShadePass] = this->m_gfxDevice->CreateRenderPass(
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
				{
					.Type = RenderPassAttachment::Type::DepthStencil,
					.LoadOp = RenderPassAttachment::LoadOpType::Load,
					.Texture = this->m_mainDepthTexture,
					.InitialLayout = RHI::ResourceStates::DepthRead,
					.SubpassLayout = RHI::ResourceStates::DepthRead,
					.FinalLayout = RHI::ResourceStates::DepthRead
				},
			}
		});
}

void PhxEngine::Renderer::RenderPath3DForward::PrepareFrameRenderData(
	ICommandList* commandList,
	Scene::CameraComponent const& mainCamera,
	Scene::Scene& scene)
{
	auto _ = commandList->BeginScopedMarker("Prepare Frame Data");

	Shader::Frame frameData = {};
	// Move to Renderer...
	frameData.BrdfLUTTexIndex = scene.GetBrdfLutDescriptorIndex();
	frameData.LightEntityDescritporIndex = RHI::cInvalidDescriptorIndex;
	frameData.LightDataOffset = 0;
	frameData.LightCount = 0;
	frameData.MatricesDescritporIndex = RHI::cInvalidDescriptorIndex;
	frameData.MatricesDataOffset = 0;
	frameData.SceneData = scene.GetShaderData();

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

void PhxEngine::Renderer::RenderPath3DForward::UpdateRTAccelerationStructures(ICommandList* commandList, PhxEngine::Scene::Scene& scene)
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

void PhxEngine::Renderer::RenderPath3DForward::RenderGeometry(RHI::ICommandList* commandList, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes)
{
	auto _ = commandList->BeginScopedMarker("Render Meshes");

	GPUAllocation instanceBufferAlloc =
		commandList->AllocateGpu(
			sizeof(Shader::ShaderMeshInstancePointer) * drawQueue.Size(),
			sizeof(Shader::ShaderMeshInstancePointer));

	// See how this data is copied over.
	const DescriptorIndex instanceBufferDescriptorIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(instanceBufferAlloc.GpuBuffer, SubresouceType::SRV);
	Shader::ShaderMeshInstancePointer* pInstancePointerData = (Shader::ShaderMeshInstancePointer*)instanceBufferAlloc.CpuData;

	struct InstanceBatch
	{
		entt::entity MeshEntity = entt::null;
		uint32_t NumInstance;
		uint32_t DataOffset;
	} instanceBatch = {};

	auto batchFlush = [&]()
	{
		if (instanceBatch.NumInstance == 0)
		{
			return;
		}

		auto [meshComponent, nameComponent] = scene.GetRegistry().get<Scene::MeshComponent, Scene::NameComponent>(instanceBatch.MeshEntity);

		std::string modelName = nameComponent.Name;
		auto scrope = commandList->BeginScopedMarker(modelName);

		auto& materiaComp = scene.GetRegistry().get<Scene::MaterialComponent>(meshComponent.Material);

		Shader::GeometryPassPushConstants pushConstant = {};
		pushConstant.GeometryIndex = meshComponent.GlobalIndexOffsetGeometryBuffer;
		pushConstant.MaterialIndex = materiaComp.GlobalBufferIndex;
		pushConstant.InstancePtrBufferDescriptorIndex = instanceBufferDescriptorIndex;
		pushConstant.InstancePtrDataOffset = instanceBatch.DataOffset;

		commandList->BindPushConstant(0, pushConstant);
		commandList->DrawIndexed(
			meshComponent.TotalIndices,
			instanceBatch.NumInstance,
			meshComponent.GlobalByteOffsetIndexBuffer);
	};

	commandList->BindIndexBuffer(scene.GetGlobalIndexBuffer());
	uint32_t instanceCount = 0;
	for (const DrawBatch& drawBatch : drawQueue.DrawItems)
	{
		entt::entity meshEntityHandle = (entt::entity)drawBatch.GetMeshEntityHandle();

		// Flush if we are dealing with a new Mesh
		if (instanceBatch.MeshEntity != meshEntityHandle)
		{
			// TODO: Flush draw
			batchFlush();

			instanceBatch.MeshEntity = meshEntityHandle;
			instanceBatch.NumInstance = 0;
			instanceBatch.DataOffset = (uint32_t)(instanceBufferAlloc.Offset + instanceCount * sizeof(Shader::ShaderMeshInstancePointer));
		}

		auto& instanceComp = scene.GetRegistry().get<Scene::MeshInstanceComponent>((entt::entity)drawBatch.GetInstanceEntityHandle());

		for (uint32_t renderCamIndex = 0; renderCamIndex < 1; renderCamIndex++)
		{
			Shader::ShaderMeshInstancePointer shaderMeshPtr = {};
			shaderMeshPtr.Create(instanceComp.GlobalBufferIndex, renderCamIndex);

			// Write into actual GPU-buffer:
			std::memcpy(pInstancePointerData + instanceCount, &shaderMeshPtr, sizeof(shaderMeshPtr)); // memcpy whole structure into mapped pointer to avoid read from uncached memory

			instanceBatch.NumInstance++;
			instanceCount++;
		}
	}

	// Flush what ever is left over.
	batchFlush();
}
