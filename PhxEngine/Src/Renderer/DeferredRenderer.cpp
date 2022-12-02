#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "DeferredRenderer.h"

#include <PhxEngine/App/Application.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Core/Math.h>
#include <Shaders/ShaderInterop.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Systems/ConsoleVarSystem.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

namespace
{
    namespace RootParameters_GBuffer
    {
        enum
        {
            PushConstant = 0,
            FrameCB,
            CameraCB,
        };
    }

    namespace RootParameters_FullQuad
    {
        enum
        {
            TextureSRV = 0,
        };
    }

    namespace RootParameters_DeferredLightingFulLQuad
    {
        enum
        {
            FrameCB = 0,
            CameraCB,
            Lights,
            GBuffer,
        };
    }

    namespace RootParameters_EnvMap_SkyProceduralCapture
    {
        enum
        {
            FrameCB = 0,
            CameraCB,
            CubeRenderCamsCB,
        };
    }

    namespace RootParameters_Shadow
    {
        enum
        {
            Push = 0,
            FrameCB,
            CameraCB,
            RenderCams,
        };
    }

    namespace RootParameters_ToneMapping
    {
        enum
        {
            Push = 0,
            HDRBuffer,
        };
    }
}

static AutoConsoleVar_Int sCVarRTShadows("Renderer.Shadows.RT", "RT_SHADOWS", 1, ConsoleVarFlags::EditReadOnly);

void DeferredRenderer::FreeResources()
{
    this->FreeTextureResources();
    
    for (RHI::BufferHandle buffer : this->m_constantBuffers)
    {
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteBuffer(buffer);
    }
}

void DeferredRenderer::FreeTextureResources()
{
    for (int i = 0; i < this->m_renderPasses.size(); i++)
    {
        if (this->m_renderPasses[i].IsValid())
        {
            IGraphicsDevice::Ptr->DeleteRenderPass(this->m_renderPasses[i]);
        }
    }

    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_depthBuffer);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.AlbedoTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.NormalTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.SurfaceTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.SpecularTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_deferredLightBuffer);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_finalColourBuffer);
}

void DeferredRenderer::Initialize()
{
    auto& spec = LayeredApplication::Ptr->GetSpec();

    this->m_cascadeShadowMaps = std::make_unique<Graphics::CascadeShadowMap>(kCascadeShadowMapRes, kCascadeShadowMapFormat, true);

    this->m_canvasSize = { static_cast<float>(spec.WindowWidth), static_cast<float>(spec.WindowHeight) };
    this->CreateRenderTargets(m_canvasSize);
    this->CreatePSOs();

    this->m_commandList = RHI::IGraphicsDevice::Ptr->CreateCommandList();
    RHI::CommandListDesc commandLineDesc = {};
    commandLineDesc.QueueType = RHI::CommandQueueType::Compute;
    this->m_computeCommandList = RHI::IGraphicsDevice::Ptr->CreateCommandList(commandLineDesc);

    // this->m_scene.BrdfLUT = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\BrdfLut.dds", true, this->m_commandList);

    // -- Create Constant Buffers ---
    {
        RHI::BufferDesc bufferDesc = {};
        bufferDesc.Usage = RHI::Usage::Default;
        bufferDesc.Binding = RHI::BindingFlags::ConstantBuffer;
        bufferDesc.SizeInBytes = sizeof(Shader::Frame);
        bufferDesc.InitialState = RHI::ResourceStates::ConstantBuffer;

        bufferDesc.DebugName = "Frame Constant Buffer";
        this->m_constantBuffers[CB_Frame] = RHI::IGraphicsDevice::Ptr->CreateBuffer(bufferDesc);
    }
}

void DeferredRenderer::CreateRenderTargets(DirectX::XMFLOAT2 const& size)
{
    // -- Depth ---
    RHI::TextureDesc desc = {};
    desc.Width = std::max(size.x, 1.0f);
    desc.Height = std::max(size.y, 1.0f);
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.IsBindless = false;

    desc.Format = RHI::FormatType::D32;
    desc.IsTypeless = true;
    desc.DebugName = "Depth Buffer";
    desc.OptmizedClearValue.DepthStencil.Depth = 0.0f;
    desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
    desc.InitialState = RHI::ResourceStates::DepthRead;
    this->m_depthBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);
    // -- Depth end ---

    // -- GBuffers ---
    desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::ShaderResource;
    desc.IsBindless = true;

    desc.Format = RHI::FormatType::RGBA32_FLOAT;
    desc.DebugName = "Albedo Buffer";
    this->m_gBuffer.AlbedoTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    // desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    desc.Format = RHI::FormatType::RGBA16_SNORM;
    desc.DebugName = "Normal Buffer";
    this->m_gBuffer.NormalTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.Format = RHI::FormatType::RGBA8_UNORM;
    desc.DebugName = "Surface Buffer";
    this->m_gBuffer.SurfaceTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.Format = RHI::FormatType::SRGBA8_UNORM;
    desc.DebugName = "Specular Buffer";
    this->m_gBuffer.SpecularTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.IsTypeless = true;
    desc.Format = RHI::FormatType::RGBA16_FLOAT;
    desc.DebugName = "Deferred Lighting";
    this->m_deferredLightBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    desc.DebugName = "Final Colour Buffer";
    this->m_finalColourBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);


    // Create backing render targets
    this->m_renderPasses[RenderPass_GBuffer] = IGraphicsDevice::Ptr->CreateRenderPass(
        {
            .Attachments =
            {
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_gBuffer.AlbedoTexture,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_gBuffer.NormalTexture,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_gBuffer.SurfaceTexture,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_gBuffer.SpecularTexture,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .Type = RenderPassAttachment::Type::DepthStencil,
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_depthBuffer,
                    .InitialLayout = RHI::ResourceStates::DepthRead,
                    .SubpassLayout = RHI::ResourceStates::DepthWrite,
                    .FinalLayout = RHI::ResourceStates::DepthRead
                },
            }
        });

    this->m_renderPasses[RenderPass_DeferredLighting] = IGraphicsDevice::Ptr->CreateRenderPass(
        {
            .Attachments =
            {
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_deferredLightBuffer,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::RenderTarget
                },
            }
        });

    this->m_renderPasses[RenderPass_Sky] = IGraphicsDevice::Ptr->CreateRenderPass(
        {
            .Attachments =
            {
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Load,
                    .Texture = this->m_deferredLightBuffer,
                    .InitialLayout = RHI::ResourceStates::RenderTarget,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .Type = RenderPassAttachment::Type::DepthStencil,
                    .LoadOp = RenderPassAttachment::LoadOpType::Load,
                    .Texture = this->m_depthBuffer,
                    .InitialLayout = RHI::ResourceStates::DepthRead,
                    .SubpassLayout = RHI::ResourceStates::DepthRead,
                    .FinalLayout = RHI::ResourceStates::DepthRead
                },
            }
        });

    this->m_renderPasses[RenderPass_PostFx] = IGraphicsDevice::Ptr->CreateRenderPass(
        {
            .Attachments =
            {
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->m_finalColourBuffer,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
            }
        });
}

void DeferredRenderer::UpdateRaytracingAccelerationStructures(PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList)
{
    if (!scene.GetTlas().IsValid())
    {
        return;
    }

    BufferHandle instanceBuffer = IGraphicsDevice::Ptr->GetRTAccelerationStructureDesc(scene.GetTlas()).TopLevel.InstanceBuffer;
    {
        auto _ = commandList->BeginScopedMarker("Copy TLAS buffer");

        BufferHandle currentTlasUploadBuffer = scene.GetTlasUploadBuffer();

        commandList->TransitionBarrier(instanceBuffer, ResourceStates::Common, ResourceStates::CopyDest);
        commandList->CopyBuffer(
            instanceBuffer,
            0,
            currentTlasUploadBuffer,
            0,
            IGraphicsDevice::Ptr->GetBufferDesc(instanceBuffer).SizeInBytes);

    }

	{
        auto _ = commandList->BeginScopedMarker("BLAS update");
		auto view = scene.GetAllEntitiesWith<MeshComponent>();
		for (auto e : view)
		{
			auto& meshComponent = view.get<MeshComponent>(e);

			if (meshComponent.Blas.IsValid())
			{
				switch (meshComponent.BlasState)
				{
                case MeshComponent::BLASState::Rebuild:
					commandList->RTBuildAccelerationStructure(meshComponent.Blas);
					break;
				case MeshComponent::BLASState::Refit:
					assert(false);
					break;
				case MeshComponent::BLASState::Complete:
				default:
					break;
				}

				meshComponent.BlasState = MeshComponent::BLASState::Complete;
			}
		}
	}

    // Move to Scene
    // TLAS:
    {
        auto _ = commandList->BeginScopedMarker("TLAS Update");
        BufferDesc instanceBufferDesc = IGraphicsDevice::Ptr->GetBufferDesc(instanceBuffer);

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
}

void DeferredRenderer::RefreshEnvProbes(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList)
{
    // Currently not considering env props
    EnvProbeComponent skyCaptureProbe;
    skyCaptureProbe.Position = camera.Eye;
    skyCaptureProbe.textureIndex = 0;

    // Render Camera
    Renderer::RenderCam cameras[6];
    Renderer::CreateCubemapCameras(
        skyCaptureProbe.Position,
        camera.ZNear,
        camera.ZFar,
        Core::Span<Renderer::RenderCam>(cameras, ARRAYSIZE(cameras)));

    RHI::ScopedMarker scope = commandList->BeginScopedMarker("Refresh Env Probes");

    commandList->BeginRenderPass(scene.GetEnvMapRenderPasses(skyCaptureProbe.textureIndex));
    commandList->SetGraphicsPSO(this->m_pso[PSO_EnvCapture_SkyProcedural]);

    RHI::Viewport v(Scene::Scene::kEnvmapRes , Scene::Scene::kEnvmapRes);
    this->m_commandList->SetViewports(&v, 1);

    RHI::Rect rec(LONG_MAX, LONG_MAX);
    this->m_commandList->SetScissors(&rec, 1);

    Shader::RenderCams renderCamsCB;
    for (int i = 0; i < ARRAYSIZE(cameras); i++)
    {
        DirectX::XMStoreFloat4x4(&renderCamsCB.ViewProjection[i], cameras[i].ViewProjection);
        renderCamsCB.Properties[i].x = i;
    }
    commandList->BindDynamicConstantBuffer(RootParameters_EnvMap_SkyProceduralCapture::CubeRenderCamsCB, renderCamsCB);

    Shader::Camera shaderCamCB = {};
    shaderCamCB.CameraPosition = skyCaptureProbe.Position;
    commandList->BindDynamicConstantBuffer(RootParameters_EnvMap_SkyProceduralCapture::CameraCB, shaderCamCB);

    commandList->BindConstantBuffer(RootParameters_EnvMap_SkyProceduralCapture::FrameCB, this->m_constantBuffers[CB_Frame]);

    // 240 is the number of vers on an ISOSphere
    // 6 instances, one per cam side.
    commandList->Draw(240, 6);

    commandList->EndRenderPass();

    // Generate MIPS

    // Bind Compute shader

    TextureHandle envMapArray = scene.GetEnvMapArray();
    {
        RHI::ScopedMarker scope = commandList->BeginScopedMarker("Generate Mip Chain");
        this->m_commandList->SetComputeState(this->m_psoCompute[PsoComputeType::PSO_GenerateMipMaps_TextureCubeArray]);

        TextureDesc textureDesc = RHI::IGraphicsDevice::Ptr->GetTextureDesc(envMapArray);

        Shader::GenerateMipChainPushConstants push = {};
        push.ArrayIndex = skyCaptureProbe.textureIndex;
        for (int i = 0; i < textureDesc.MipLevels - 1; i++)
        {
            GpuBarrier preBarriers[] =
            {
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(preBarriers, ARRAYSIZE(preBarriers)));

            textureDesc.Width = std::max(1u, textureDesc.Width / 2);
            textureDesc.Height = std::max(1u, textureDesc.Height / 2);

            push.TextureInput = IGraphicsDevice::Ptr->GetDescriptorIndex(envMapArray, RHI::SubresouceType::SRV, i);
            push.TextureOutput = IGraphicsDevice::Ptr->GetDescriptorIndex(envMapArray, RHI::SubresouceType::UAV, i + 1);
            push.OutputResolution = { (float)textureDesc.Width, (float)textureDesc.Height };
            push.OutputResolutionRcp = { 1.0f / (float)textureDesc.Width, 1.0f / (float)textureDesc.Height };
            this->m_commandList->BindPushConstant(0, push);

            this->m_commandList->Dispatch(
                std::max(1u, (textureDesc.Width + Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE - 1) / Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE),
                std::max(1u, (textureDesc.Height + Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE - 1) / Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE),
                6);

            GpuBarrier postBarriers[] =
            {
                GpuBarrier::CreateMemory(),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(postBarriers, ARRAYSIZE(postBarriers)));

        }
    }

    {
        RHI::ScopedMarker scope = commandList->BeginScopedMarker("Filter Env Map");
        this->m_commandList->SetComputeState(this->m_psoCompute[PsoComputeType::PSO_FilterEnvMap]);

        TextureDesc textureDesc = RHI::IGraphicsDevice::Ptr->GetTextureDesc(envMapArray);

        textureDesc.Width = 1;
        textureDesc.Height = 1;

        Shader::FilterEnvMapPushConstants push = {};
        push.ArrayIndex = skyCaptureProbe.textureIndex;
        // We can skip the most detailed mip as it's bassiclly a straight reflection. So end before 0.
        for (int i = textureDesc.MipLevels - 1; i > 0; --i)
        {
            GpuBarrier preBarriers[] =
            {
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(preBarriers, ARRAYSIZE(preBarriers)));


            push.TextureInput = IGraphicsDevice::Ptr->GetDescriptorIndex(envMapArray, RHI::SubresouceType::SRV, std::max(0, (int)i - 2));
            push.TextureOutput = IGraphicsDevice::Ptr->GetDescriptorIndex(envMapArray, RHI::SubresouceType::UAV, i);
            push.FilteredResolution = { (float)textureDesc.Width, (float)textureDesc.Height };
            push.FilteredResolutionRcp = { 1.0f / (float)textureDesc.Width, 1.0f / (float)textureDesc.Height };
            push.FilterRoughness = (float)i / (float)textureDesc.MipLevels;
            // In the example they use 1024: https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
            push.NumSamples = 128;
            this->m_commandList->BindPushConstant(0, push);


            this->m_commandList->Dispatch(
                (textureDesc.Width + Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE - 1) / Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE,
                (textureDesc.Height + Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE - 1) / Shader::GENERATE_MIP_CHAIN_2D_BLOCK_SIZE,
                6);

            GpuBarrier postBarriers[] =
            {
                GpuBarrier::CreateMemory(),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(postBarriers, ARRAYSIZE(postBarriers)));

            textureDesc.Width *= 2;
            textureDesc.Height *= 2;
        }
    }

    // Filter Env Map
}

void PhxEngine::Renderer::DeferredRenderer::DrawMeshes(DrawQueue const& drawQueue, PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList, uint32_t numInstances)
{
    auto scrope = commandList->BeginScopedMarker("Render Scene Meshes");

    GPUAllocation instanceBufferAlloc =
        commandList->AllocateGpu(
            sizeof(Shader::ShaderMeshInstancePointer) * drawQueue.Size(),
            sizeof(Shader::ShaderMeshInstancePointer));

    // See how this data is copied over.
    const DescriptorIndex instanceBufferDescriptorIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(instanceBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);

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

        auto [meshComponent, nameComponent] = scene.GetRegistry().get<MeshComponent, NameComponent>(instanceBatch.MeshEntity);
        commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);

        std::string modelName = nameComponent.Name;

        auto scrope = commandList->BeginScopedMarker(modelName);
        for (size_t i = 0; i < meshComponent.Surfaces.size(); i++)
        {
            auto& materiaComp = scene.GetRegistry().get<MaterialComponent>(meshComponent.Surfaces[i].Material);

            Shader::GeometryPassPushConstants pushConstant = {};
            pushConstant.GeometryIndex = meshComponent.GlobalGeometryBufferIndex + i;
            pushConstant.MaterialIndex = materiaComp.GlobalBufferIndex;
            pushConstant.InstancePtrBufferDescriptorIndex = instanceBufferDescriptorIndex;
            pushConstant.InstancePtrDataOffset = instanceBatch.DataOffset;

            commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

            commandList->DrawIndexed(
                meshComponent.Surfaces[i].NumIndices,
                instanceBatch.NumInstance,
                meshComponent.Surfaces[i].IndexOffsetInMesh);
        }
    };

    uint32_t instanceCount = 0;
    for (const DrawBatch& drawBatch : drawQueue.DrawBatches)
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

        auto& instanceComp = scene.GetRegistry().get<MeshInstanceComponent>((entt::entity)drawBatch.GetInstanceEntityHandle());

        // TODO: Check if visible to camera or cameras
        Shader::ShaderMeshInstancePointer shaderMeshPtr = {};
        shaderMeshPtr.InstanceIndex = instanceComp.GlobalBufferIndex;
        instanceBatch.NumInstance++;
        instanceCount++;
    }

    // Flush what ever is left over.
    batchFlush();
}

void DeferredRenderer::PrepareFrameRenderData(
    RHI::CommandListHandle commandList,
    CameraComponent const& mainCamera,
    PhxEngine::Scene::Scene& scene)
{
    auto scope = this->m_commandList->BeginScopedMarker("Prepare Frame Data");

    GPUAllocation lightBufferAlloc =
        commandList->AllocateGpu(
            sizeof(Shader::ShaderLight) * Shader::SHADER_LIGHT_ENTITY_COUNT,
            sizeof(Shader::ShaderLight));
    GPUAllocation matrixBufferAlloc =
        commandList->AllocateGpu(
            sizeof(DirectX::XMMATRIX) * Shader::MATRIX_COUNT,
            sizeof(DirectX::XMMATRIX));

    Shader::ShaderLight* lightArray = (Shader::ShaderLight*)lightBufferAlloc.CpuData;
    DirectX::XMMATRIX* matrixArray = (DirectX::XMMATRIX*)matrixBufferAlloc.CpuData;

    auto lightView = scene.GetAllEntitiesWith<LightComponent, TransformComponent>();
    size_t lightCount = 0;
    size_t matrixCount = 0;
    for (auto e : lightView)
    {
        auto [lightComponent, transformComponent] = lightView.get<LightComponent, TransformComponent>(e);
        if (!lightComponent.IsEnabled() && lightCount < Shader::SHADER_LIGHT_ENTITY_COUNT)
        {
            continue;
        }

        Shader::ShaderLight* renderLight = lightArray + lightCount++;
        renderLight->SetType(lightComponent.Type);
        renderLight->SetRange(lightComponent.Range);
        renderLight->SetIntensity(lightComponent.Intensity);
        renderLight->SetFlags(lightComponent.Flags);
        renderLight->SetDirection(lightComponent.Direction);
        renderLight->ColorPacked = Math::PackColour(lightComponent.Colour);
        renderLight->SetIndices(matrixCount);
        renderLight->SetNumCascades(0);
        renderLight->Position = transformComponent.GetPosition();

        if (lightComponent.Type == LightComponent::kSpotLight)
        {

            const float outerConeAngle = lightComponent.OuterConeAngle;
            const float innerConeAngle = std::min(lightComponent.InnerConeAngle, outerConeAngle);
            const float outerConeAngleCos = std::cos(outerConeAngle);
            const float innerConeAngleCos = std::cos(innerConeAngle);

            // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#inner-and-outer-cone-angles
            const float lightAngleScale = 1.0f / std::max(0.001f, innerConeAngleCos - outerConeAngleCos);
            const float lightAngleOffset = -outerConeAngleCos * lightAngleScale;

            renderLight->SetConeAngleCos(outerConeAngleCos);
            renderLight->SetAngleScale(lightAngleScale);
            renderLight->SetAngleOffset(lightAngleOffset);
        }

        if (lightComponent.CastShadows())
        {
            switch (lightComponent.Type)
            {
            case LightComponent::kDirectionalLight:
            {
                // Add support for adjusting the number of cascades
                std::vector<Renderer::RenderCam> renderCams = this->m_cascadeShadowMaps->CreateRenderCams(mainCamera, lightComponent, 800.0f);
                renderLight->SetNumCascades((uint32_t)renderCams.size());
                renderLight->CascadeTextureIndex = this->m_cascadeShadowMaps->GetTextureArrayIndex();

                for (size_t i = 0; i < renderCams.size(); i++)
                {
                    std:memcpy(matrixArray + matrixCount, &renderCams[i].ViewProjection, sizeof(DirectX::XMMATRIX));
                    matrixCount++;
                }
                break;
            }
            case LightComponent::kOmniLight:
            {
                /*
                std::array<DirectX::XMFLOAT4X4, 6> matrices;
                Shadow::ConstructOmniLightMatrices(lightComponent, matrices);

                for (size_t i = 0; i < matrices.size(); i++)
                {
                    this->m_matricesCPUData.emplace_back(matrices[i]);
                }

                // Not sure I understand this math here or why the cubemap depth needs to be re-mapped
                const float nearZ = 0.1f;	// watch out: reversed depth buffer! Also, light near plane is constant for simplicity, this should match on cpu side!
                const float farZ = std::max(1.0f, lightComponent.Range); // watch out: reversed depth buffer!
                const float fRange = farZ / (farZ - nearZ);
                const float cubemapDepthRemapNear = fRange;
                const float cubemapDepthRemapFar = -fRange * nearZ;
                renderLight.SetCubemapDepthRemapNear(cubemapDepthRemapNear);
                renderLight.SetCubemapDepthRemapFar(cubemapDepthRemapFar);
                */
                break;
            }
            case LightComponent::kSpotLight:
            {
                // TODO:
                break;
            }
            }
        }
	}

	Shader::Frame frameData = {};
    // Move to Renderer...
    frameData.BrdfLUTTexIndex = scene.GetBrdfLutDescriptorIndex();
    frameData.LightEntityDescritporIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(lightBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
    frameData.LightDataOffset = lightBufferAlloc.Offset;
    frameData.LightCount = lightCount;

    frameData.MatricesDescritporIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(matrixBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
    frameData.MatricesDataOffset = matrixBufferAlloc.Offset;

	// Upload data
	RHI::GpuBarrier preCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
	};
	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

	commandList->WriteBuffer(this->m_constantBuffers[CB_Frame], frameData);

    commandList->CopyBuffer(
        scene.GetInstanceBuffer(),
        0,
        scene.GetInstanceUploadBuffer(),
        0,
        scene.GetNumInstances() * sizeof(Shader::Geometry));

	commandList->CopyBuffer(
		scene.GetGeometryBuffer(),
		0,
		scene.GetGeometryUploadBuffer(),
		0,
		scene.GetNumGeometryEntries() * sizeof(Shader::Geometry));

	commandList->CopyBuffer(
		scene.GetMaterialBuffer(),
		0,
		scene.GetMaterialUploadBuffer(),
		0,
		scene.GetNumMaterialEntries() * sizeof(Shader::MaterialData));

	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
	};

	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
}

void DeferredRenderer::CreatePSOs()
{
    this->m_pso[PSO_GBufferPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_GBufferPass),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_GBufferPass),
            .DepthStencilRenderState = {.DepthFunc = ComparisonFunc::Greater },
            .RtvFormats = {
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).Format,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).Format,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).Format,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SpecularTexture).Format,
            },
            .DsvFormat = { IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).Format }
        });

    this->m_pso[PSO_Sky] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_Sky),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_SkyProcedural),
            .DepthStencilRenderState = {
                .DepthWriteEnable = false,
                .DepthFunc = RHI::ComparisonFunc::GreaterOrEqual
            },
            .RasterRenderState = { .DepthClipEnable = false },
            .RtvFormats = { IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).Format },
            .DsvFormat = { IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).Format }
        });


    this->m_pso[PSO_FullScreenQuad] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_FullscreenQuad),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_FullscreenQuad),
            .DepthStencilRenderState = {
                .DepthTestEnable = false,
            },
            .RtvFormats = { IGraphicsDevice::Ptr->GetTextureDesc(IGraphicsDevice::Ptr->GetBackBuffer()).Format },
        });

    this->m_pso[PSO_ToneMappingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_ToneMapping),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_ToneMapping),
            .DepthStencilRenderState = {
                .DepthTestEnable = false,
            },
            .RtvFormats = { IGraphicsDevice::Ptr->GetTextureDesc(IGraphicsDevice::Ptr->GetBackBuffer()).Format },
        });

    this->m_pso[PSO_DeferredLightingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_DeferredLighting),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_DeferredLighting),
            .DepthStencilRenderState = {
                .DepthTestEnable = false,
            },
            .RtvFormats = { IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).Format },
        });

    this->m_pso[PSO_DeferredLightingPass_RTShadows] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_DeferredLighting),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_DeferredLighting_RTShadows),
            .DepthStencilRenderState = {
                .DepthTestEnable = false,
            },
            .RtvFormats = { IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).Format },
        });

    assert(IGraphicsDevice::Ptr->CheckCapability(DeviceCapability::RT_VT_ArrayIndex_Without_GS));
    this->m_pso[PSO_EnvCapture_SkyProcedural] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_EnvMap_Sky),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_EnvMap_SkyProcedural),
            .DepthStencilRenderState = { .DepthFunc = ComparisonFunc::GreaterOrEqual },
            .RasterRenderState = {.CullMode = RasterCullMode::Front }, // Since we are inside the ICOSphere, chance cull mode.
            .RtvFormats = { Scene::Scene::kEnvmapFormat },
            .DsvFormat = { Scene::Scene::kEnvmapDepth }
        });

    this->m_pso[PSO_Shadow] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_ShadowPass),
            .DepthStencilRenderState =
                {
                    .DepthFunc = ComparisonFunc::Greater,
                },
            .RasterRenderState =
                {
                    .CullMode = RasterCullMode::None,
                    .DepthClipEnable = 1,
                    .DepthBias = -1,
                    .DepthBiasClamp = 0,
                    .SlopeScaledDepthBias = -4,
                },
            .DsvFormat = { kCascadeShadowMapFormat }
        });

    // Compute PSO's
    this->m_psoCompute[PSO_GenerateMipMaps_TextureCubeArray] = IGraphicsDevice::Ptr->CreateComputePso(
        {
            .ComputeShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::CS_GenerateMips_TextureCubeArray)
        });
    this->m_psoCompute[PSO_FilterEnvMap] = IGraphicsDevice::Ptr->CreateComputePso(
        {
            .ComputeShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::CS_FilterEnvMap)
        });
}

void DeferredRenderer::RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene)
{
    this->m_commandList->Open();

    this->PrepareFrameRenderData(this->m_commandList, camera, scene);

    if (IGraphicsDevice::Ptr->CheckCapability(DeviceCapability::RayTracing))
    {
        this->UpdateRaytracingAccelerationStructures(scene, this->m_commandList);
    }

    // Preframe Transisions
    {
        this->m_commandList->BeginScopedMarker("PrePass Transisions");

    }
    this->RefreshEnvProbes(camera, scene, this->m_commandList);

    Shader::Camera cameraData = {};
    cameraData.CameraPosition = camera.Eye;
    cameraData.ViewProjection = camera.ViewProjection;
    cameraData.ViewProjectionInv = camera.ViewProjectionInv;
    cameraData.ProjInv = camera.ProjectionInv;
    cameraData.ViewInv = camera.ViewInv;

    /* Disabled Light
    auto view = scene.GetAllEntitiesWith<LightComponent>();
    bool foundDirectionalLight = false;
    for (auto e : view)
    {
        // Only support Directional Lights
        auto& lightComponent = view.get<LightComponent>(e);

        if (lightComponent.Type != LightComponent::kDirectionalLight || !lightComponent.CastShadows())
        {
            continue;
        }

        // only suppport one light
        if (foundDirectionalLight)
        {
            break;
        }

        foundDirectionalLight = true;

        auto scrope = this->m_commandList->BeginScopedMarker("Shadow Map Pass");

        // -- Prepare PSO ---
        this->m_commandList->BeginRenderPass(this->m_cascadeShadowMaps->GetRenderPass());
        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_Shadow]);

        RHI::Viewport v(kCascadeShadowMapRes, kCascadeShadowMapRes);
        this->m_commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        this->m_commandList->SetScissors(&rec, 1);

        // -- Bind Data ---
        std::vector<Renderer::RenderCam> renderCams = this->m_cascadeShadowMaps->CreateRenderCams(camera, lightComponent, 800.0f);

        Shader::RenderCams renderCamsCB;
        for (int i = 0; i < renderCams.size(); i++)
        {
            DirectX::XMStoreFloat4x4(&renderCamsCB.ViewProjection[i], renderCams[i].ViewProjection);
            renderCamsCB.Properties[i].x = i;
        }

        // Do NOT NEED FRAME AND CAM DATA!!
        this->m_commandList->BindConstantBuffer(RootParameters_Shadow::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_Shadow::CameraCB, cameraData);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_Shadow::RenderCams, renderCamsCB);

        // DrawMeshes(scene, this->m_commandList, Graphics::CascadeShadowMap::GetNumCascades());
        this->m_commandList->EndRenderPass();
    }
    */
    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque GBuffer Pass");

        // -- Prepare PSO ---
        this->m_commandList->BeginRenderPass(this->m_renderPasses[RenderPass_GBuffer]);
        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_GBufferPass]);

        RHI::Viewport v(this->m_canvasSize.x, this->m_canvasSize.y);
        this->m_commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        this->m_commandList->SetScissors(&rec, 1);

        // -- Bind Data ---

        this->m_commandList->BindConstantBuffer(RootParameters_GBuffer::FrameCB, this->m_constantBuffers[CB_Frame]);
        // TODO: Create a camera const buffer as well
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_GBuffer::CameraCB, cameraData);

        // this->m_commandList->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        // Construct a render queue
        thread_local static DrawQueue drawQueue;

        // Look through Meshes and instances?
        // TODO: Only get a list of visible entries
        auto instanceView = scene.GetAllEntitiesWith<MeshInstanceComponent>();
        for (auto e : instanceView)
        {
            auto& instanceComponent = instanceView.get<MeshInstanceComponent>(e);

            // TODO: Set up distance need AABB to calculate this.
            const float distance = 0;

            drawQueue.Push((uint32_t)instanceComponent.Mesh, (uint32_t)e, distance);
        }

        DrawMeshes(drawQueue, scene, this->m_commandList);
        this->m_commandList->EndRenderPass();
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque Deferred Lighting Pass");

        this->m_commandList->BeginRenderPass(this->m_renderPasses[RenderPass_DeferredLighting]);
        if ((bool)sCVarRTShadows.Get())
        {
            this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_DeferredLightingPass_RTShadows]);
        }
        else
        {
            this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_DeferredLightingPass]);
        }

        this->m_commandList->BindConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB, cameraData);


        this->m_commandList->BindDynamicDescriptorTable(
            RootParameters_DeferredLightingFulLQuad::GBuffer,
            {
                this->m_depthBuffer,
                this->m_gBuffer.AlbedoTexture,
                this->m_gBuffer.NormalTexture,
                this->m_gBuffer.SurfaceTexture,
                this->m_gBuffer.SpecularTexture,
            });
        this->m_commandList->Draw(3, 1, 0, 0);

        this->m_commandList->EndRenderPass();
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Draw Sky Pass");

        this->m_commandList->BeginRenderPass(this->m_renderPasses[RenderPass_Sky]);
        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_Sky]);

        this->m_commandList->BindConstantBuffer(DefaultRootParameters::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(DefaultRootParameters::CameraCB, cameraData);

        this->m_commandList->SetRenderTargets({ this->m_deferredLightBuffer }, this->m_depthBuffer);

        this->m_commandList->Draw(3, 1, 0, 0);
        this->m_commandList->EndRenderPass();
    }

	{
        auto _ = this->m_commandList->BeginScopedMarker("Post Process FX");
		{
			auto scrope = this->m_commandList->BeginScopedMarker("Tone Mapping");

            this->m_commandList->BeginRenderPass(this->m_renderPasses[RenderPass_PostFx]);
			this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_ToneMappingPass]);

            // Exposure, not needed right now
            this->m_commandList->BindPushConstant(RootParameters_ToneMapping::Push, 1.0f);

            this->m_commandList->BindDynamicDescriptorTable(
                RootParameters_ToneMapping::HDRBuffer,
                {
                    this->m_deferredLightBuffer,
                });

            this->m_commandList->Draw(3, 1, 0, 0);
            this->m_commandList->EndRenderPass();
		}
	}

    this->m_commandList->Close();
    RHI::IGraphicsDevice::Ptr->ExecuteCommandLists(this->m_commandList.get());
}

void DeferredRenderer::OnWindowResize(DirectX::XMFLOAT2 const& size)
{
    this->FreeTextureResources();

    this->CreateRenderTargets(size);
    this->m_canvasSize = size;
}
