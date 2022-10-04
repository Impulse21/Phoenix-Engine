#include "DeferredSceneRenderer.h"

#include <PhxEngine/App/Application.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Core/Math.h>
#include <Shaders/ShaderInterop.h>
#include <PhxEngine/Renderer/Renderer.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

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
    namespace RootParameters_ToneMapping
    {
        enum
        {
            Push = 0,
            HDRBuffer,
        };
    }
}

void DeferredRenderer::FreeResources()
{
    this->FreeTextureResources();
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteBuffer(this->m_geometryGpuBuffer);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteBuffer(this->m_materialGpuBuffer);
    
    for (RHI::BufferHandle buffer : this->m_constantBuffers)
    {
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteBuffer(buffer);
    }

    for (RHI::BufferHandle buffer : this->m_resourceBuffers)
    {
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteBuffer(buffer);
    }

    for (int i = 0; i < this->m_materialUploadBuffers.size(); i++)
    {
        if (this->m_materialUploadBuffers[i].IsValid())
        {
            IGraphicsDevice::Ptr->DeleteBuffer(this->m_materialUploadBuffers[i]);
        }
    }
    for (int i = 0; i < this->m_geometryUploadBuffers.size(); i++)
    {
        if (this->m_geometryUploadBuffers[i].IsValid())
        {
            IGraphicsDevice::Ptr->DeleteBuffer(this->m_geometryUploadBuffers[i]);
        }
    }

    for (int i = 0; i < this->m_envMapRenderPasses.size(); i++)
    {
        if (this->m_envMapRenderPasses[i].IsValid())
        {
            IGraphicsDevice::Ptr->DeleteRenderPass(this->m_envMapRenderPasses[i]);
        }
    }

    if (this->m_envMapDepthBuffer.IsValid())
    {
        IGraphicsDevice::Ptr->DeleteTexture(this->m_envMapDepthBuffer);
    }

    if (this->m_envMapArray.IsValid())
    {
        IGraphicsDevice::Ptr->DeleteTexture(this->m_envMapArray);
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

    {
        RHI::BufferDesc bufferDesc = {};
        bufferDesc.Binding = RHI::BindingFlags::ShaderResource;
        bufferDesc.InitialState = RHI::ResourceStates::ShaderResource;
        bufferDesc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        bufferDesc.SizeInBytes = sizeof(Shader::ShaderLight) * Shader::SHADER_LIGHT_ENTITY_COUNT;
        bufferDesc.StrideInBytes = sizeof(Shader::ShaderLight);
        bufferDesc.DebugName = "Light Entity Buffer";

        this->m_resourceBuffers[RB_LightEntities] = RHI::IGraphicsDevice::Ptr->CreateBuffer(bufferDesc);
    }

    {
        RHI::BufferDesc bufferDesc = {};
        bufferDesc.Binding = RHI::BindingFlags::ShaderResource;
        bufferDesc.InitialState = RHI::ResourceStates::ShaderResource;
        bufferDesc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        bufferDesc.SizeInBytes = sizeof(DirectX::XMFLOAT4X4) * Shader::MATRIX_COUNT;
        bufferDesc.StrideInBytes = sizeof(DirectX::XMFLOAT4X4);
        bufferDesc.DebugName = "Matrix Buffer";

        this->m_resourceBuffers[RB_Matrices] = RHI::IGraphicsDevice::Ptr->CreateBuffer(bufferDesc);
    }

    this->m_materialUploadBuffers.resize(IGraphicsDevice::Ptr->GetMaxInflightFrames());
    this->m_geometryUploadBuffers.resize(IGraphicsDevice::Ptr->GetMaxInflightFrames());
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

    commandList->BeginRenderPass(this->m_envMapRenderPasses[skyCaptureProbe.textureIndex]);
    commandList->SetGraphicsPSO(this->m_pso[PSO_EnvCapture_SkyProcedural]);

    RHI::Viewport v(kEnvmapRes, kEnvmapRes);
    this->m_commandList->SetViewports(&v, 1);

    RHI::Rect rec(LONG_MAX, LONG_MAX);
    this->m_commandList->SetScissors(&rec, 1);

    Shader::CubemapRenderCams renderCamsCB;
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
    {
        RHI::ScopedMarker scope = commandList->BeginScopedMarker("Generate Mip Chain");
        this->m_commandList->SetComputeState(this->m_psoCompute[PsoComputeType::PSO_GenerateMipMaps_TextureCubeArray]);

        TextureDesc textureDesc = RHI::IGraphicsDevice::Ptr->GetTextureDesc(this->m_envMapArray);

        Shader::GenerateMipChainPushConstants push = {};
        push.ArrayIndex = skyCaptureProbe.textureIndex;
        for (int i = 0; i < textureDesc.MipLevels - 1; i++)
        {
            GpuBarrier preBarriers[] =
            {
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(preBarriers, ARRAYSIZE(preBarriers)));

            textureDesc.Width = std::max(1u, textureDesc.Width / 2);
            textureDesc.Height = std::max(1u, textureDesc.Height / 2);

            push.TextureInput = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::SRV, i);
            push.TextureOutput = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::UAV, i + 1);
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
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(postBarriers, ARRAYSIZE(postBarriers)));

        }
    }

    {
        RHI::ScopedMarker scope = commandList->BeginScopedMarker("Filter Env Map");
        this->m_commandList->SetComputeState(this->m_psoCompute[PsoComputeType::PSO_FilterEnvMap]);

        TextureDesc textureDesc = RHI::IGraphicsDevice::Ptr->GetTextureDesc(this->m_envMapArray);

        textureDesc.Width = 1;
        textureDesc.Height = 1;

        Shader::FilterEnvMapPushConstants push = {};
        push.ArrayIndex = skyCaptureProbe.textureIndex;
        // We can skip the most detailed mip as it's bassiclly a straight reflection. So end before 0.
        for (int i = textureDesc.MipLevels - 1; i > 0; --i)
        {
            GpuBarrier preBarriers[] =
            {
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(this->m_envMapArray, textureDesc.InitialState, ResourceStates::UnorderedAccess, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(preBarriers, ARRAYSIZE(preBarriers)));


            push.TextureInput = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::SRV, std::max(0, (int)i - 2));
            push.TextureOutput = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::UAV, i);
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
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 0),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 1),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 2),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 3),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 4),
                GpuBarrier::CreateTexture(this->m_envMapArray, ResourceStates::UnorderedAccess, textureDesc.InitialState, i + 1, push.ArrayIndex * 6 + 5),
            };

            this->m_commandList->TransitionBarriers(Core::Span<GpuBarrier>(postBarriers, ARRAYSIZE(postBarriers)));

            textureDesc.Width *= 2;
            textureDesc.Height *= 2;
        }
    }

    // Filter Env Map
}

void DeferredRenderer::DrawMeshes(PhxEngine::Scene::Scene& scene, RHI::CommandListHandle commandList)
{
    auto scrope = commandList->BeginScopedMarker("Render Scene Meshes");

    auto view = scene.GetAllEntitiesWith<MeshRenderComponent, NameComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto [meshRenderComponent, nameComp, transformComponent] = view.get<MeshRenderComponent, NameComponent, TransformComponent>(entity);

        // Skip non opaque items
        if ((meshRenderComponent.RenderBucketMask & MeshRenderComponent::RenderType::RenderType_Opaque) != MeshRenderComponent::RenderType::RenderType_Opaque)
        {
            continue;
        }

        Assets::Mesh& mesh = *meshRenderComponent.Mesh;
        commandList->BindIndexBuffer(mesh.IndexGpuBuffer);

        std::string modelName = nameComp.Name;

        auto scrope = commandList->BeginScopedMarker(modelName);
        for (size_t i = 0; i < mesh.Surfaces.size(); i++)
        {
            Shader::GeometryPassPushConstants pushConstant = {};
            // pushConstant.MeshIndex = scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
            pushConstant.GeometryIndex = mesh.Surfaces[i].GlobalBufferIndex;
            pushConstant.WorldTransform = transformComponent.WorldMatrix;
            commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

            commandList->DrawIndexed(
                mesh.Surfaces[i].NumIndices,
                1,
                mesh.Surfaces[i].IndexOffsetInMesh);
        }
    }
}

void DeferredRenderer::RunProbeUpdateSystem(PhxEngine::Scene::Scene& scene)
{
    if (!this->m_envMapArray.IsValid())
	{
        this->m_envMapDepthBuffer = IGraphicsDevice::Ptr->CreateTexture(
            {
                .BindingFlags = BindingFlags::DepthStencil,
                .Dimension = TextureDimension::TextureCube,
                .InitialState = ResourceStates::DepthWrite,
                .Format = kEnvmapDepth,
                .IsBindless = false,
                .Width = kEnvmapRes,
                .Height = kEnvmapRes,
                .ArraySize = 6,
                .OptmizedClearValue = {.DepthStencil = {.Depth = 0.0f }},
                .DebugName = "Env Map Depth"
			});

        // Create EnvMap

        this->m_envMapArray = IGraphicsDevice::Ptr->CreateTexture(
            {
                .BindingFlags = BindingFlags::ShaderResource | BindingFlags::UnorderedAccess | BindingFlags::RenderTarget,
                .Dimension = TextureDimension::TextureCubeArray,
                .InitialState = ResourceStates::RenderTarget,
                .Format = kEnvmapFormat,
                .IsBindless = true,
                .Width = kEnvmapRes,
                .Height = kEnvmapRes,
                .ArraySize = kEnvmapCount * 6,
                .MipLevels = kEnvmapMIPs,
                .OptmizedClearValue = {.Colour = { 0.0f, 0.0f, 0.0f, 1.0f }},
                .DebugName = "Env Map Array"
            });

        // Create sub resource views

        // Cube arrays per mip level:
        for (int i = 0; i < kEnvmapMIPs; i++)
        {
            int subIndex = IGraphicsDevice::Ptr->CreateSubresource(
                this->m_envMapArray,
                RHI::SubresouceType::SRV,
                0,
                kEnvmapCount * 6,
                i,
                1);

            assert(i == subIndex);
            subIndex = IGraphicsDevice::Ptr->CreateSubresource(
                this->m_envMapArray,
                RHI::SubresouceType::UAV,
                0,
                kEnvmapCount * 6,
                i,
                1);
            assert(i == subIndex);
        }
        
        // individual cubes with mips:
        for (int i = 0; i < kEnvmapCount; i++)
        {
			int subIndex = IGraphicsDevice::Ptr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::SRV,
				i * 6,
				6,
				0,
				~0u);
            assert((kEnvmapMIPs + i) == subIndex);
        }

	    // individual cubes only mip0:
        for (int i = 0; i < kEnvmapCount; i++)
		{
			int subIndex = IGraphicsDevice::Ptr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::SRV,
				i * 6,
				6,
				0,
				1);
            assert((kEnvmapMIPs + kEnvmapCount + i) == subIndex);
        }

		// Create Render Passes
		for (uint32_t i = 0; i < kEnvmapCount; ++i)
		{
			int subIndex = IGraphicsDevice::Ptr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::RTV,
				i * 6,
				6,
				0,
				1);

                assert(i == subIndex);

				this->m_envMapRenderPasses[i] = IGraphicsDevice::Ptr->CreateRenderPass(
					{
			            .Attachments =
			            {
				            {
                                .LoadOp = RenderPassAttachment::LoadOpType::DontCare,
                                .Texture = this->m_envMapArray,
                                .Subresource = subIndex,
                                .StoreOp = RenderPassAttachment::StoreOpType::Store,
					            .InitialLayout = RHI::ResourceStates::RenderTarget,
					            .SubpassLayout = RHI::ResourceStates::RenderTarget,
					            .FinalLayout = RHI::ResourceStates::RenderTarget
				            },
				            {
					            .Type = RenderPassAttachment::Type::DepthStencil,
					            .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                                .Texture = this->m_envMapDepthBuffer,
                                .StoreOp = RenderPassAttachment::StoreOpType::DontCare,
					            .InitialLayout = RHI::ResourceStates::DepthWrite,
					            .SubpassLayout = RHI::ResourceStates::DepthWrite,
					            .FinalLayout = RHI::ResourceStates::DepthWrite
				            },
			            }
					});
            }
    }

    // TODO: Future env probe stuff
}

void DeferredRenderer::PrepareFrameRenderData(
    RHI::CommandListHandle commandList,
    PhxEngine::Scene::Scene& scene)
{
    auto scope = this->m_commandList->BeginScopedMarker("Prepare Frame Data");

    // Update Geometry Count
    std::unordered_set<Assets::StandardMaterial*> foundMaterials;
    auto view = scene.GetAllEntitiesWith<MeshRenderComponent>();
    this->m_numGeometryEntires = 0;
    this->m_numMaterialEntries = 0;
    for (auto e : view)
    {
        auto comp = view.get<MeshRenderComponent>(e);
        this->m_numGeometryEntires += comp.Mesh->Surfaces.size();

        for (Assets::Mesh::SurfaceDesc surface : comp.Mesh->Surfaces)
        {
            if (foundMaterials.find(surface.Material.get()) == foundMaterials.end())
            {
                // I believe as long as there isn't to many hash collisions, this should perform okay.
                // Would prefer to just have this data in a flat list somewhere however.
                foundMaterials.insert(surface.Material.get());
            }
        }
    }

    this->m_numMaterialEntries = foundMaterials.size();

    if (!this->m_materialGpuBuffer.IsValid() ||
        IGraphicsDevice::Ptr->GetBufferDesc(this->m_materialGpuBuffer).SizeInBytes != this->m_numMaterialEntries * sizeof(Shader::MaterialData))
    {
        RHI::BufferDesc desc = {};
        desc.DebugName = "Material Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.InitialState = ResourceStates::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        desc.CreateBindless = true;
        desc.StrideInBytes = sizeof(Shader::MaterialData);
        desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_numMaterialEntries;

        if (this->m_materialGpuBuffer.IsValid())
        {
            IGraphicsDevice::Ptr->DeleteBuffer(this->m_materialGpuBuffer);
        }
        this->m_materialGpuBuffer = IGraphicsDevice::Ptr->CreateBuffer(desc);

        desc.CreateBindless = false;
        desc.DebugName = "Material Upload Data";
        desc.Usage = RHI::Usage::Upload;
        desc.Binding = RHI::BindingFlags::None;
        desc.MiscFlags = RHI::BufferMiscFlags::None;
        desc.InitialState = ResourceStates::CopySource;
        for (int i = 0; i < this->m_materialUploadBuffers.size(); i++)
        {
            if (this->m_materialUploadBuffers[i].IsValid())
            {
                IGraphicsDevice::Ptr->DeleteBuffer(this->m_materialUploadBuffers[i]);
            }
            this->m_materialUploadBuffers[i] = IGraphicsDevice::Ptr->CreateBuffer(desc);
        }
    }

    // Update Material Syste
	BufferHandle currMaterialUploadBuffer = this->m_materialUploadBuffers[IGraphicsDevice::Ptr->GetFrameIndex()];
	Shader::MaterialData* materialBufferMappedData = (Shader::MaterialData*)IGraphicsDevice::Ptr->GetBufferMappedData(currMaterialUploadBuffer);
	uint32_t currMat = 0;
	for (Assets::StandardMaterial* mat : foundMaterials)
	{
        if (!mat)
        {
            continue;
        }

		Shader::MaterialData* shaderData = materialBufferMappedData + currMat;

        shaderData->AlbedoColour = { mat->Albedo.x, mat->Albedo.y, mat->Albedo.z };
        shaderData->EmissiveColourPacked = Core::Math::PackColour(mat->Emissive);
        shaderData->AO = mat->Ao;
        shaderData->Metalness = mat->Metalness;
        shaderData->Roughness = mat->Roughness;

        shaderData->AlbedoTexture = RHI::cInvalidDescriptorIndex;
        if (mat->AlbedoTexture && mat->AlbedoTexture->GetRenderHandle().IsValid())
        {
            shaderData->AlbedoTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->AlbedoTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
        }

        shaderData->AOTexture = RHI::cInvalidDescriptorIndex;
        if (mat->AoTexture && mat->AoTexture->GetRenderHandle().IsValid())
        {
            shaderData->AOTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->AoTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
        }

        shaderData->MaterialTexture = RHI::cInvalidDescriptorIndex;
        if (mat->MetalRoughnessTexture && mat->MetalRoughnessTexture->GetRenderHandle().IsValid())
        {
            shaderData->MaterialTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->MetalRoughnessTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
            assert(shaderData->MaterialTexture != RHI::cInvalidDescriptorIndex);

            // shaderData->MetalnessTexture = mat->MetalRoughnessTexture->GetDescriptorIndex();
            // shaderData->RoughnessTexture = mat->MetalRoughnessTexture->GetDescriptorIndex();
        }

        shaderData->NormalTexture = RHI::cInvalidDescriptorIndex;
        if (mat->NormalMapTexture && mat->NormalMapTexture->GetRenderHandle().IsValid())
        {
            shaderData->NormalTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->NormalMapTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
        }
        mat->GlobalBufferIndex = currMat++;
	}

    // Recreate Buffers for geometry data
    if (!this->m_geometryGpuBuffer.IsValid() ||
        IGraphicsDevice::Ptr->GetBufferDesc(this->m_geometryGpuBuffer).SizeInBytes != (this->m_numGeometryEntires * sizeof(Shader::Geometry)))
    {
        RHI::BufferDesc desc = {};
        desc.DebugName = "Geometry Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.InitialState = ResourceStates::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        desc.CreateBindless = true;
        desc.StrideInBytes = sizeof(Shader::Geometry);
        desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_numGeometryEntires;

        if (this->m_geometryGpuBuffer.IsValid())
        {
            IGraphicsDevice::Ptr->DeleteBuffer(this->m_geometryGpuBuffer);
        }
        this->m_geometryGpuBuffer = IGraphicsDevice::Ptr->CreateBuffer(desc);

        desc.DebugName = "Geometry Upload";
        desc.Usage = RHI::Usage::Upload;
        desc.Binding = RHI::BindingFlags::None;
        desc.MiscFlags = RHI::BufferMiscFlags::None;
        desc.InitialState = ResourceStates::CopySource;
        desc.CreateBindless = false;
        for (int i = 0; i < this->m_geometryUploadBuffers.size(); i++)
        {
            if (this->m_geometryUploadBuffers[i].IsValid())
            {
                IGraphicsDevice::Ptr->DeleteBuffer(this->m_geometryUploadBuffers[i]);
            }
            this->m_geometryUploadBuffers[i] = IGraphicsDevice::Ptr->CreateBuffer(desc);
        }
    }

    // Move to a "System" like function at some point UpdateMeshSystems
    BufferHandle currentGeoUploadBuffer = this->m_geometryUploadBuffers[IGraphicsDevice::Ptr->GetFrameIndex()];
    Shader::Geometry* geometryBufferMappedData = (Shader::Geometry*)IGraphicsDevice::Ptr->GetBufferMappedData(currentGeoUploadBuffer);
    uint32_t currGeoIndex = 0;
    for (auto e : view)
    {
        auto comp = view.get<MeshRenderComponent>(e);
        Assets::Mesh& mesh = *comp.Mesh;
        for (int i = 0; i < mesh.Surfaces.size(); i++)
        {
            Assets::Mesh::SurfaceDesc& surfaceDesc = mesh.Surfaces[i];
            Shader::Geometry* geometryShaderData = geometryBufferMappedData + currGeoIndex;
            // Material Index setup
            geometryShaderData->MaterialIndex = surfaceDesc.Material->GlobalBufferIndex;
            geometryShaderData->NumIndices = surfaceDesc.NumIndices;
            geometryShaderData->NumVertices = surfaceDesc.NumVertices;
            geometryShaderData->IndexOffset = surfaceDesc.IndexOffsetInMesh;
            geometryShaderData->VertexBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(mesh.VertexGpuBuffer, RHI::SubresouceType::SRV);
            geometryShaderData->PositionOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Position).ByteOffset;
            geometryShaderData->TexCoordOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::TexCoord).ByteOffset;
            geometryShaderData->NormalOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Normal).ByteOffset;
            geometryShaderData->TangentOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Tangent).ByteOffset;
            surfaceDesc.GlobalBufferIndex = currGeoIndex++;
        }
	}



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
        renderLight->Indices = this->m_matricesCPUData.size();

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
        /*
        switch (lightComponent.Type)
        {
        case LightComponent::kDirectionalLight:
        {
            // std::array<DirectX::XMFLOAT4X4, kNumShadowCascades> matrices;
            // auto& cameraComponent = *this->m_scene.Cameras.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);
            // Shadow::ConstructDirectionLightMatrices(cameraComponent, lightComponent, matrices);

            for (size_t i = 0; i < matrices.size(); i++)
            {
                this->m_matricesCPUData.emplace_back(matrices[i]);
            }
            break;
        }
        case LightComponent::kOmniLight:
        {
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

            break;
        }
        case LightComponent::kSpotLight:
		{
			// TODO:
			break;
		}
		}
		*/
	}

	Shader::Frame frameData = {};
	frameData.SceneData.NumLights = lightCount;
	frameData.SceneData.GeometryBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_geometryGpuBuffer, RHI::SubresouceType::SRV);
	frameData.SceneData.MaterialBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_materialGpuBuffer, RHI::SubresouceType::SRV);
	frameData.SceneData.LightEntityIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_resourceBuffers[RB_LightEntities], RHI::SubresouceType::SRV);
	frameData.SceneData.MatricesIndex = RHI::cInvalidDescriptorIndex;

    frameData.SceneData.AtmosphereData = {};
    auto worldEnvView = scene.GetRegistry().view<WorldEnvironmentComponent>();
    if (worldEnvView.empty())
    {
        WorldEnvironmentComponent worldComp = {};
        frameData.SceneData.AtmosphereData.ZenithColour = worldComp.ZenithColour;
        frameData.SceneData.AtmosphereData.HorizonColour = worldComp.HorizonColour;
        frameData.SceneData.AtmosphereData.AmbientColour = worldComp.AmbientColour;
    }
    else
    {
        auto& worldComp = worldEnvView.get<WorldEnvironmentComponent>(worldEnvView[0]);

        frameData.SceneData.AtmosphereData.ZenithColour = worldComp.ZenithColour;
        frameData.SceneData.AtmosphereData.HorizonColour = worldComp.HorizonColour;
        frameData.SceneData.AtmosphereData.AmbientColour = worldComp.AmbientColour;

    }
    frameData.SceneData.EnvMapArray = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::SRV);
    frameData.SceneData.EnvMap_NumMips = kEnvmapMIPs;
    frameData.BrdfLUTTexIndex = scene.GetBrdfLutDescriptorIndex();

	// Upload data
	RHI::GpuBarrier preCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_materialGpuBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_LightEntities], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
	};
	commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

	commandList->WriteBuffer(this->m_constantBuffers[CB_Frame], frameData);

	commandList->CopyBuffer(
		this->m_geometryGpuBuffer,
		0,
		currentGeoUploadBuffer,
		0,
		this->m_numGeometryEntires * sizeof(Shader::Geometry));

	commandList->CopyBuffer(
		this->m_materialGpuBuffer,
		0,
		currMaterialUploadBuffer,
		0,
		this->m_numMaterialEntries * sizeof(Shader::MaterialData));

    commandList->CopyBuffer(
        this->m_resourceBuffers[RB_LightEntities],
        0,
        lightBufferAlloc.GpuBuffer,
        lightBufferAlloc.Offset,
        Shader::SHADER_LIGHT_ENTITY_COUNT * sizeof(Shader::ShaderLight));

	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateBuffer(this->m_materialGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_LightEntities], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
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

    assert(IGraphicsDevice::Ptr->CheckCapability(DeviceCapability::RT_VT_ArrayIndex_Without_GS));
    this->m_pso[PSO_EnvCapture_SkyProcedural] = IGraphicsDevice::Ptr->CreateGraphicsPSO(
        {
            .VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_EnvMap_Sky),
            .PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_EnvMap_SkyProcedural),
            .DepthStencilRenderState = { .DepthFunc = ComparisonFunc::GreaterOrEqual },
            .RasterRenderState = {.CullMode = RasterCullMode::Front }, // Since we are inside the ICOSphere, chance cull mode.
            .RtvFormats = { kEnvmapFormat },
            .DsvFormat = { kEnvmapDepth }
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

void DeferredRenderer::Update(PhxEngine::Scene::Scene& scene)
{
    this->RunProbeUpdateSystem(scene);
}

void DeferredRenderer::RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene)
{
    this->m_commandList->Open();

    this->PrepareFrameRenderData(this->m_commandList, scene);

    this->RefreshEnvProbes(camera, scene, this->m_commandList);

    Shader::Camera cameraData = {};
    cameraData.CameraPosition = camera.Eye;
    cameraData.ViewProjection = camera.ViewProjection;
    cameraData.ViewProjectionInv = camera.ViewProjectionInv;
    cameraData.ProjInv = camera.ProjectionInv;
    cameraData.ViewInv = camera.ViewInv;

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

        DrawMeshes(scene, this->m_commandList);
        this->m_commandList->EndRenderPass();
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque Deferred Lighting Pass");

        this->m_commandList->BeginRenderPass(this->m_renderPasses[RenderPass_DeferredLighting]);
        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_DeferredLightingPass]);

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
