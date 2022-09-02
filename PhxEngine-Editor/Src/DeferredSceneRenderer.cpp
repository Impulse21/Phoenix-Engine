#include "DeferredSceneRenderer.h"

#include <PhxEngine/App/Application.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Core/Math.h>

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

void DeferredRenderer::DrawMeshes(PhxEngine::Scene::New::Scene& scene, RHI::CommandListHandle commandList)
{
    auto scrope = commandList->BeginScopedMarker("Render Scene Meshes");

    auto view = scene.GetAllEntitiesWith<New::MeshRenderComponent, New::NameComponent, New::TransformComponent>();
    for (auto entity : view)
    {

        auto [meshRenderComponent, nameComp, transformComponent] = view.get<New::MeshRenderComponent, New::NameComponent, New::TransformComponent>(entity);;

        // Skip non opaque items
        if ((meshRenderComponent.RenderBucketMask & New::MeshRenderComponent::RenderType::RenderType_Opaque) != New::MeshRenderComponent::RenderType::RenderType_Opaque)
        {
            continue;
        }

        Assets::Mesh& mesh = *meshRenderComponent.Mesh;
        commandList->BindIndexBuffer(mesh.IndexGpuBuffer);

        std::string modelName = nameComp.Name;

        auto scrope = commandList->BeginScopedMarker(modelName);
        for (size_t i = 0; i < mesh.Surfaces.size(); i++)
        {
            Shader::GeometryPassPushConstants pushConstant;
            // pushConstant.MeshIndex = scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
            pushConstant.GeometryIndex = mesh.Surfaces[i].GlobalBufferIndex;
            TransposeMatrix(transformComponent.WorldMatrix, &pushConstant.WorldTransform);
            commandList->BindPushConstant(RootParameters_GBuffer::PushConstant, pushConstant);

            commandList->DrawIndexed(
                mesh.Surfaces[i].NumIndices,
                1,
                mesh.Surfaces[i].IndexOffsetInMesh);
        }
    }
}

void DeferredRenderer::PrepareFrameRenderData(
    RHI::CommandListHandle commandList,
    PhxEngine::Scene::New::Scene& scene,
    Shader::Frame& outFrameShaderData)
{
    // Update Geometry Count
    std::unordered_set<Assets::StandardMaterial*> foundMaterials;
    auto view = scene.GetAllEntitiesWith<New::MeshRenderComponent>();
    for (auto e : view)
    {
        auto comp = view.get<New::MeshRenderComponent>(e);
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
        desc.StrideInBytes = sizeof(Shader::Geometry);
        desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_numMaterialEntries;

        if (this->m_materialGpuBuffer.IsValid())
        {
            IGraphicsDevice::Ptr->DeleteBuffer(this->m_geometryGpuBuffer);
        }
        this->m_materialGpuBuffer = IGraphicsDevice::Ptr->CreateBuffer(desc);

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
		Shader::MaterialData* shaderData = materialBufferMappedData + currMat;

        shaderData->AlbedoColour = { mat->Albedo.x, mat->Albedo.y, mat->Albedo.z };
        shaderData->EmissiveColourPacked = Core::Math::PackColour(mat->Emissive);
        shaderData->AO = mat->Ao;

        shaderData->AlbedoTexture = RHI::cInvalidDescriptorIndex;
        if (mat->AlbedoTexture && mat->AlbedoTexture->GetRenderHandle().IsValid())
        {
            shaderData->AlbedoTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->AlbedoTexture->GetRenderHandle());
        }

        shaderData->AOTexture = RHI::cInvalidDescriptorIndex;
        if (mat->AoTexture && mat->AoTexture->GetRenderHandle().IsValid())
        {
            shaderData->AOTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->AoTexture->GetRenderHandle());
        }

        shaderData->MaterialTexture = RHI::cInvalidDescriptorIndex;
        if (mat->MetalRoughnessTexture && mat->MetalRoughnessTexture->GetRenderHandle().IsValid())
        {
            shaderData->MaterialTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->NormalMapTexture->GetRenderHandle());
            assert(shaderData->MaterialTexture != RHI::cInvalidDescriptorIndex);

            // shaderData->MetalnessTexture = mat->MetalRoughnessTexture->GetDescriptorIndex();
            // shaderData->RoughnessTexture = mat->MetalRoughnessTexture->GetDescriptorIndex();
        }

        shaderData->NormalTexture = RHI::cInvalidDescriptorIndex;
        if (mat->NormalMapTexture && mat->NormalMapTexture->GetRenderHandle().IsValid())
        {
            shaderData->NormalTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->NormalMapTexture->GetRenderHandle());
        }
        mat->GlobalBufferIndex = currMat++;
	}

    // Recreate Buffers for geometry data
    if (!this->m_geometryGpuBuffer.IsValid() ||
        IGraphicsDevice::Ptr->GetBufferDesc(this->m_geometryGpuBuffer).SizeInBytes != this->m_numGeometryEntires * sizeof(Shader::Geometry))
    {
        RHI::BufferDesc desc = {};
        desc.DebugName = "Geometry Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.InitialState = ResourceStates::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
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
        auto comp = view.get<New::MeshRenderComponent>(e);
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
            geometryShaderData->VertexBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(mesh.VertexGpuBuffer);
            geometryShaderData->PositionOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Position).ByteOffset;
            geometryShaderData->TexCoordOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::TexCoord).ByteOffset;
            geometryShaderData->NormalOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Normal).ByteOffset;
            geometryShaderData->TangentOffset = mesh.GetVertexAttribute(Assets::Mesh::VertexAttribute::Tangent).ByteOffset;
            currGeoIndex++;
        }
	}

    // Upload data
    RHI::GpuBarrier preCopyBarriers[] =
    {
        RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        RHI::GpuBarrier::CreateBuffer(this->m_materialGpuBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
    };
    commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));
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
        this->m_numGeometryEntires * sizeof(Shader::Geometry));

    RHI::GpuBarrier postCopyBarriers[] =
    {
        RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        RHI::GpuBarrier::CreateBuffer(this->m_materialGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
    };
    commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));

    // TODO: I am here
    
    // const XMFLOAT2 atlasDIMRcp = XMFLOAT2(1.0f / float(this->m_shadowAtlas->GetDesc().Width), 1.0f / float(this->m_shadowAtlas->GetDesc().Height));
    /*
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
    */
    outFrameShaderData.Scene.NumLights = this->m_lightCpuData.size();
    outFrameShaderData.Scene.GeometryBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_geometryGpuBuffer);
    outFrameShaderData.Scene.MaterialBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_materialGpuBuffer);
    outFrameShaderData.LightEntityIndex = RHI::cInvalidDescriptorIndex; // this->m_resourceBuffers[RB_LightEntities]->GetDescriptorIndex();
    outFrameShaderData.MatricesIndex = RHI::cInvalidDescriptorIndex;
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

void DeferredRenderer::RenderScene(Scene::New::CameraComponent const& camera, Scene::New::Scene& scene)
{
    this->m_commandList->Open();

    Shader::Frame frameData = {};
    frameData.BrdfLUTTexIndex = cInvalidDescriptorIndex;// this->m_scene.BrdfLUT->GetDescriptorIndex();
    this->PrepareFrameRenderData(this->m_commandList, scene, frameData);

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