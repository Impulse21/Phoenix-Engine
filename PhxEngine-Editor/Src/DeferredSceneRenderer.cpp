#include "DeferredSceneRenderer.h"

#include <PhxEngine/App/Application.h>
#include <PhxEngine/Scene/Components.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

using namespace PhxEngine::RHI;

namespace
{
    inline void TransposeMatrix(DirectX::XMFLOAT4X4 const& in, DirectX::XMFLOAT4X4* out)
    {
        auto matrix = DirectX::XMLoadFloat4x4(&in);

        DirectX::XMStoreFloat4x4(out, DirectX::XMMatrixTranspose(matrix));
    }

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
}


void DeferredRenderer::Initialize()
{
    auto& spec = LayeredApplication::Ptr->GetSpec();
    this->CreatePSOs();
    this->m_canvasSize = { static_cast<float>(spec.WindowWidth), static_cast<float>(spec.WindowHeight) };
    this->CreateRenderTargets(m_canvasSize);

    this->m_commandList = RHI::IGraphicsDevice::Ptr->CreateCommandList();
    RHI::CommandListDesc commandLineDesc = {};
    commandLineDesc.QueueType = RHI::CommandQueueType::Compute;
    this->m_computeCommandList = RHI::IGraphicsDevice::Ptr->CreateCommandList(commandLineDesc);

    // TODO: Use Asset Store
    // this->m_scene.BrdfLUT = this->m_textureCache->LoadTexture("..\\Assets\\Textures\\IBL\\BrdfLut.dds", true, this->m_commandList);


    // -- Create Constant Buffers ---
    {
        RHI::BufferDesc bufferDesc = {};
        bufferDesc.CpuAccessMode = RHI::CpuAccessMode::Default;
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
    desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
    desc.BindingFlags = desc.BindingFlags | RHI::BindingFlags::DepthStencil;
    desc.InitialState = RHI::ResourceStates::DepthWrite;

    this->m_depthBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    // -- GBuffers ---
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::RenderTarget;

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

    desc.Format = RHI::FormatType::RGBA32_FLOAT;
    desc.DebugName = "Debug Position Buffer";

    // TODO: Determine what format should be used here.
    desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    desc.DebugName = "Deferred Lighting";
    this->m_deferredLightBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);
}

void DeferredRenderer::DrawMeshes(PhxEngine::Scene::New::Scene const& scene, RHI::CommandListHandle commandList)
{
    auto scrope = commandList->BeginScopedMarker("Render Scene Meshes");

    for (int i = 0; i < scene.MeshInstances.GetCount(); i++)
    {
        auto& meshInstanceComponent = scene.MeshInstances[i];

        if ((meshInstanceComponent.RenderBucketMask & RenderType::RenderType_Opaque) != RenderType::RenderType_Opaque)
        {
            ECS::Entity e = scene.MeshInstances.GetEntity(i);
            auto& meshComponent = *scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
            std::string modelName = "UNKNOWN";
            auto* nameComponent = scene.Names.GetComponent(meshInstanceComponent.MeshId);
            if (nameComponent)
            {
                modelName = nameComponent->Name;
            }
            continue;
        }

        ECS::Entity e = scene.MeshInstances.GetEntity(i);
        auto& meshComponent = *scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
        auto& transformComponent = *scene.Transforms.GetComponent(e);
        commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);

        std::string modelName = "UNKNOWN";
        auto* nameComponent = scene.Names.GetComponent(meshInstanceComponent.MeshId);
        if (nameComponent)
        {
            modelName = nameComponent->Name;
        }

        auto scrope = commandList->BeginScopedMarker(modelName);
        for (size_t i = 0; i < meshComponent.Geometry.size(); i++)
        {
            Shader::GeometryPassPushConstants pushConstant;
            pushConstant.MeshIndex = scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
            pushConstant.GeometryIndex = meshComponent.Geometry[i].GlobalGeometryIndex;
            TransposeMatrix(transformComponent.WorldMatrix, &pushConstant.WorldTransform);
            commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

            commandList->DrawIndexed(
                meshComponent.Geometry[i].NumIndices,
                1,
                meshComponent.Geometry[i].IndexOffsetInMesh);
        }
    }
}

void DeferredRenderer::PrepareFrameRenderData(RHI::CommandListHandle commandList)
{
    size_t numGeometry = 0ull;
    for (int i = 0; i < Meshes.GetCount(); i++)
    {
        MeshComponent& mesh = Meshes[i];
        mesh.CreateRenderData(graphicsDevice, commandList);

        // Determine the number of geometry data
        numGeometry += mesh.Geometry.size();
    }

    // Construct Geometry Data
    // TODO: Create a thread to collect geometry Count
    size_t geometryCounter = 0ull;
    if (this->m_geometryShaderData.size() != numGeometry)
    {
        this->m_geometryShaderData.resize(numGeometry);

        size_t globalGeomtryIndex = 0ull;
        for (int i = 0; i < Meshes.GetCount(); i++)
        {
            MeshComponent& mesh = Meshes[i];

            for (int j = 0; j < mesh.Geometry.size(); j++)
            {
                // Construct the Geometry data
                auto& geometryData = mesh.Geometry[j];
                geometryData.GlobalGeometryIndex = geometryCounter++;

                auto& geometryShaderData = this->m_geometryShaderData[geometryData.GlobalGeometryIndex];

                geometryShaderData.MaterialIndex = this->Materials.GetIndex(geometryData.MaterialID);
                geometryShaderData.NumIndices = geometryData.NumIndices;
                geometryShaderData.NumVertices = geometryData.NumVertices;
                geometryShaderData.IndexOffset = geometryData.IndexOffsetInMesh;
                geometryShaderData.VertexBufferIndex = mesh.VertexGpuBuffer->GetDescriptorIndex();
                geometryShaderData.PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
                geometryShaderData.TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
                geometryShaderData.NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
                geometryShaderData.TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
            }
        }

        // -- Create GPU Data ---
        BufferDesc desc = {};
        desc.DebugName = "Geometry Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        desc.StrideInBytes = sizeof(Shader::Geometry);
        desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_geometryShaderData.size();

        this->GeometryGpuBuffer = graphicsDevice->CreateBuffer(desc);

        // Upload Data
        commandList->TransitionBarrier(this->GeometryGpuBuffer, ResourceStates::Common, ResourceStates::CopyDest);
        commandList->WriteBuffer(this->GeometryGpuBuffer, this->m_geometryShaderData);
        commandList->TransitionBarrier(this->GeometryGpuBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
    }

    // Construct Material Data
    if (this->Materials.GetCount() != this->m_materialShaderData.size())
    {
        // Create Material Data
        this->m_materialShaderData.resize(this->Materials.GetCount());

        for (int i = 0; i < this->Materials.GetCount(); i++)
        {
            auto& gpuMaterial = this->m_materialShaderData[i];
            this->Materials[i].PopulateShaderData(gpuMaterial);
        }

        // -- Create GPU Data ---
        BufferDesc desc = {};
        desc.DebugName = "Material Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
        desc.StrideInBytes = sizeof(Shader::MaterialData);
        desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_materialShaderData.size();

        this->MaterialBuffer = graphicsDevice->CreateBuffer(desc);

        // Upload Data
        commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::Common, ResourceStates::CopyDest);
        commandList->WriteBuffer(this->MaterialBuffer, this->m_materialShaderData);
        commandList->TransitionBarrier(this->MaterialBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
    }

    this->m_lightCpuData.clear();
    this->m_matricesCPUData.clear();

    // const XMFLOAT2 atlasDIMRcp = XMFLOAT2(1.0f / float(this->m_shadowAtlas->GetDesc().Width), 1.0f / float(this->m_shadowAtlas->GetDesc().Height));
    for (int i = 0; i < this->m_scene.Lights.GetCount(); i++)
    {
        auto& lightComponent = this->m_scene.Lights[i];
        if (lightComponent.IsEnabled())
        {
            Shader::ShaderLight& renderLight = this->m_lightCpuData.emplace_back();
            renderLight.SetType(lightComponent.Type);
            renderLight.SetRange(lightComponent.Range);
            renderLight.SetEnergy(lightComponent.Energy);
            renderLight.SetFlags(lightComponent.Flags);
            renderLight.SetDirection(lightComponent.Direction);
            renderLight.ColorPacked = Math::PackColour(lightComponent.Colour);
            renderLight.Indices = this->m_matricesCPUData.size();

            auto& transformComponent = *this->m_scene.Transforms.GetComponent(this->m_scene.Lights.GetEntity(i));
            renderLight.Position = transformComponent.GetPosition();

            /*
            renderLight.ShadowAtlasMulAdd.x = lightComponent.ShadowRect.w * atlasDIMRcp.x;
            renderLight.ShadowAtlasMulAdd.y = lightComponent.ShadowRect.h * atlasDIMRcp.y;
            renderLight.ShadowAtlasMulAdd.z = lightComponent.ShadowRect.x * atlasDIMRcp.x;
            renderLight.ShadowAtlasMulAdd.w = lightComponent.ShadowRect.y * atlasDIMRcp.y;
            */
            switch (lightComponent.Type)
            {
            case LightComponent::kDirectionalLight:
            {
                std::array<DirectX::XMFLOAT4X4, kNumShadowCascades> matrices;
                auto& cameraComponent = *this->m_scene.Cameras.GetComponent(this->m_cameraEntities[this->m_selectedCamera]);
                Shadow::ConstructDirectionLightMatrices(cameraComponent, lightComponent, matrices);

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
        }
    }

    frameData.Scene.NumLights = this->m_lightCpuData.size();

    frameData.LightEntityIndex = this->m_resourceBuffers[RB_LightEntities]->GetDescriptorIndex();
    frameData.MatricesIndex = this->m_resourceBuffers[RB_Matrices]->GetDescriptorIndex();

    // Upload new data

    RHI::GpuBarrier barriers[] =
    {
        RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], this->m_constantBuffers[CB_Frame]->GetDesc().InitialState, RHI::ResourceStates::CopyDest),
        RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_LightEntities], this->m_resourceBuffers[RB_LightEntities]->GetDesc().InitialState, RHI::ResourceStates::CopyDest),
        RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_Matrices], this->m_resourceBuffers[RB_Matrices]->GetDesc().InitialState, RHI::ResourceStates::CopyDest),
    };

    commandList->TransitionBarriers(Span<RHI::GpuBarrier>(barriers, _countof(barriers)));

    commandList->WriteBuffer(this->m_constantBuffers[CB_Frame], frameData);
    if (!this->m_lightCpuData.empty())
    {
        commandList->WriteBuffer(this->m_resourceBuffers[RB_LightEntities], this->m_lightCpuData);
    }

    if (!this->m_matricesCPUData.empty())
    {
        commandList->WriteBuffer(this->m_resourceBuffers[RB_Matrices], this->m_matricesCPUData);
    }

    barriers[0] = RHI::GpuBarrier::CreateBuffer(this->m_constantBuffers[CB_Frame], RHI::ResourceStates::CopyDest, this->m_constantBuffers[CB_Frame]->GetDesc().InitialState);
    barriers[1] = RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_LightEntities], RHI::ResourceStates::CopyDest, this->m_resourceBuffers[RB_LightEntities]->GetDesc().InitialState);
    barriers[2] = RHI::GpuBarrier::CreateBuffer(this->m_resourceBuffers[RB_Matrices], RHI::ResourceStates::CopyDest, this->m_resourceBuffers[RB_Matrices]->GetDesc().InitialState);

    commandList->TransitionBarriers(Span<RHI::GpuBarrier>(barriers, _countof(barriers)));

}

void DeferredRenderer::CreatePSOs()
{
    {
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_GBufferPass);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_GBufferPass);
        psoDesc.InputLayout = nullptr;

        // psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::None;
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).Format);
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).Format);
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).Format);
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer._PostionTexture).Format);
        psoDesc.DsvFormat = IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).Format;

        this->m_pso[PSO_GBufferPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
    }

    {
        /*
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_ShadowPass);
        psoDesc.InputLayout = nullptr;

        psoDesc.DsvFormat = kShadowAtlasFormat;
        psoDesc.RasterRenderState.DepthBias = 100000;
        psoDesc.RasterRenderState.DepthBiasClamp = 0.0f;
        psoDesc.RasterRenderState.SlopeScaledDepthBias = 1.0f;
        psoDesc.RasterRenderState.DepthClipEnable = false;

        this->m_pso[PSO_] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
        */
    }

    {
        // TODO: Use the files directly
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_FullscreenQuad);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_FullscreenQuad);
        psoDesc.InputLayout = nullptr;
        psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::Front;
        psoDesc.DepthStencilRenderState.DepthTestEnable = false;
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(IGraphicsDevice::Ptr->GetBackBuffer()).Format);
        this->m_pso[PSO_FullScreenQuad] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);

        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_DeferredLighting);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_DeferredLighting);

        this->m_pso[PSO_DeferredLightingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
    }
}

void DeferredRenderer::RenderScene(Scene::New::CameraComponent const& camera, Scene::New::Scene const& scene)
{
    this->m_commandList->Open();

    Shader::Frame frameData = {};
    frameData.BrdfLUTTexIndex = cInvalidDescriptorIndex;// this->m_scene.BrdfLUT->GetDescriptorIndex();
    frameData.Scene.MaterialBufferIndex = this->m_materialGpuBuffers->GetDescriptorIndex();
    frameData.Scene.GeometryBufferIndex = this->m_geometryGpuBuffers->GetDescriptorIndex();

    this->PrepareFrameRenderData(this->m_commandList);

    Shader::Camera cameraData = {};
    cameraData.CameraPosition = camera.Eye;
    TransposeMatrix(camera.ViewProjection, &cameraData.ViewProjection);
    TransposeMatrix(camera.ProjectionInv, &cameraData.ProjInv);
    TransposeMatrix(camera.ViewInv, &cameraData.ViewInv);

    {
        auto _ = this->m_commandList->BeginScopedMarker("Clear Render Targets");
        this->m_commandList->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0.0f);
        this->m_commandList->ClearTextureFloat(
            this->m_gBuffer.AlbedoTexture,
            IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).OptmizedClearValue.Colour);
        this->m_commandList->ClearTextureFloat(
            this->m_gBuffer.NormalTexture,
            IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).OptmizedClearValue.Colour);
        this->m_commandList->ClearTextureFloat(
            this->m_gBuffer.SurfaceTexture,
            IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).OptmizedClearValue.Colour);
        this->m_commandList->ClearTextureFloat(
            this->m_gBuffer._PostionTexture,
            IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer._PostionTexture).OptmizedClearValue.Colour);

    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque GBuffer Pass");

        // -- Prepare PSO ---
        this->m_commandList->SetRenderTargets(
            {
                this->m_gBuffer.AlbedoTexture,
                this->m_gBuffer.NormalTexture,
                this->m_gBuffer.SurfaceTexture,
                this->m_gBuffer._PostionTexture,
            },
            this->m_depthBuffer);
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
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Opaque Deferred Lighting Pass");

        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_DeferredLightingPass]);

        // A second upload........ not ideal
        this->m_commandList->BindConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB, cameraData);

        // -- Lets go ahead and do the lighting pass now >.<
        this->m_commandList->BindDynamicStructuredBuffer(
            RootParameters_DeferredLightingFulLQuad::Lights,
            this->m_lightCpuData);

        this->m_commandList->SetRenderTargets({ this->m_deferredLightBuffer }, TextureHandle());


        RHI::GpuBarrier preTransition[] =
        {
            RHI::GpuBarrier::CreateTexture(
                this->m_depthBuffer,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).InitialState,
                RHI::ResourceStates::ShaderResource),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.AlbedoTexture,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).InitialState,
                RHI::ResourceStates::ShaderResource),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.NormalTexture,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).InitialState,
                RHI::ResourceStates::ShaderResource),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.SurfaceTexture,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).InitialState,
                RHI::ResourceStates::ShaderResource),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer._PostionTexture,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer._PostionTexture).InitialState,
                RHI::ResourceStates::ShaderResource),
        };

        this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preTransition, _countof(preTransition)));

        this->m_commandList->BindDynamicDescriptorTable(
            RootParameters_DeferredLightingFulLQuad::GBuffer,
            {
                this->m_depthBuffer,
                this->m_gBuffer.AlbedoTexture,
                this->m_gBuffer.NormalTexture,
                this->m_gBuffer.SurfaceTexture,
                this->m_gBuffer._PostionTexture,
            });
        this->m_commandList->Draw(3, 1, 0, 0);


        RHI::GpuBarrier postTransition[] =
        {
            RHI::GpuBarrier::CreateTexture(
                this->m_depthBuffer,
                RHI::ResourceStates::ShaderResource,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).InitialState),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.AlbedoTexture,
                RHI::ResourceStates::ShaderResource,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).InitialState),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.NormalTexture,
                RHI::ResourceStates::ShaderResource,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).InitialState),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer.SurfaceTexture,
                RHI::ResourceStates::ShaderResource,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).InitialState),

            RHI::GpuBarrier::CreateTexture(
                this->m_gBuffer._PostionTexture,
                RHI::ResourceStates::ShaderResource,
                IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer._PostionTexture).InitialState),
        };
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
