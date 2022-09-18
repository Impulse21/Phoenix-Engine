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

    if (this->m_envMapArray.IsValid())
    {
        IGraphicsDevice::Ptr->DeleteTexture(this->m_envMapArray);
    }
}

void DeferredRenderer::FreeTextureResources()
{
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_depthBuffer);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.AlbedoTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.NormalTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.SurfaceTexture);
    PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer._PostionTexture);
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
    desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
    desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
    desc.InitialState = RHI::ResourceStates::DepthWrite;
    this->m_depthBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);
    // -- Depth end ---

    // -- GBuffers ---
    desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::RenderTarget;
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

    desc.Format = RHI::FormatType::RGBA32_FLOAT;
    desc.DebugName = "Debug Position Buffer";
    this->m_gBuffer._PostionTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.IsTypeless = true;
    desc.Format = RHI::FormatType::RGBA16_FLOAT;
    desc.DebugName = "Deferred Lighting";
    this->m_deferredLightBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

    desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    desc.DebugName = "Final Colour Buffer";
    this->m_finalColourBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);
}

void DeferredRenderer::RefreshEnvProbs(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList)
{
    // Currently not considering env props
    EnvProbeComponent skyCaptureProbe;
    skyCaptureProbe.Position = camera.Eye;
    skyCaptureProbe.textureIndex = 0;

    // Render Camera
    Renderer::RenderCam cameras[6];
    Renderer::CreateCubemapCameras(skyCaptureProbe.Position, camera.ZNear, camera.ZFar, Core::Span<Renderer::RenderCam>(cameras, ARRAYSIZE(cameras)));

    // TODO: I am herer
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
            TransposeMatrix(transformComponent.WorldMatrix, &pushConstant.WorldTransform);
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
        // Create EnvMap
        TextureDesc desc;
        desc.ArraySize = kEnvmapCount * 6;
        desc.BindingFlags = BindingFlags::ShaderResource | BindingFlags::UnorderedAccess | BindingFlags::RenderTarget;
        desc.Format = kEnvmapFormat;
        desc.Height = kEnvmapRes;
        desc.Height = kEnvmapRes;
        desc.MipLevels = kEnvmapRes;
        desc.Dimension = TextureDimension::TextureCubeArray;
        desc.InitialState = ResourceStates::ShaderResource;
        desc.DebugName = "envMapArray";

        this->m_envMapArray = IGraphicsDevice::Ptr->CreateTexture(desc);
    }

    // TODO: Future env probe stuff
}

void DeferredRenderer::PrepareFrameRenderData(
    RHI::CommandListHandle commandList,
    PhxEngine::Scene::Scene& scene)
{
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
		Shader::MaterialData* shaderData = materialBufferMappedData + currMat;

        shaderData->AlbedoColour = { mat->Albedo.x, mat->Albedo.y, mat->Albedo.z };
        shaderData->EmissiveColourPacked = Core::Math::PackColour(mat->Emissive);
        shaderData->AO = mat->Ao;
        shaderData->Metalness = mat->Metalness;
        shaderData->Roughness = mat->Roughness;

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
            shaderData->MaterialTexture = RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(mat->MetalRoughnessTexture->GetRenderHandle());
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
            geometryShaderData->VertexBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(mesh.VertexGpuBuffer);
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
        auto& [lightComponent, transformComponent] = lightView.get<LightComponent, TransformComponent>(e);
        if (!lightComponent.IsEnabled() && lightCount < Shader::SHADER_LIGHT_ENTITY_COUNT)
        {
            continue;
        }

        Shader::ShaderLight* renderLight = lightArray + lightCount++;
        renderLight->SetType(lightComponent.Type);
        renderLight->SetRange(lightComponent.Range);
        renderLight->SetEnergy(lightComponent.Energy);
        renderLight->SetFlags(lightComponent.Flags);
        renderLight->SetDirection(lightComponent.Direction);
        renderLight->ColorPacked = Math::PackColour(lightComponent.Colour);
        renderLight->Indices = this->m_matricesCPUData.size();

        renderLight->Position = transformComponent.GetPosition();
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
	frameData.BrdfLUTTexIndex = cInvalidDescriptorIndex;// this->m_scene.BrdfLUT->GetDescriptorIndex();
	frameData.SceneData.NumLights = lightCount;
	frameData.SceneData.GeometryBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_geometryGpuBuffer);
	frameData.SceneData.MaterialBufferIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_materialGpuBuffer);
	frameData.SceneData.LightEntityIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_resourceBuffers[RB_LightEntities]);
	frameData.SceneData.MatricesIndex = RHI::cInvalidDescriptorIndex;
    frameData.SceneData.AtmosphereData = {};
    frameData.SceneData.AtmosphereData.ZenithColour = { 1.0f, 0.0f, 0.0f};// { 0.117647, 0.156863, 0.235294 };
    frameData.SceneData.AtmosphereData.HorizonColour = { 0.0f, 0.0f, 1.0f };// { 0.0392157, 0.0392157, 0.0784314 };

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

        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_Sky);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_SkyProcedural);
        psoDesc.InputLayout = nullptr;

        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).Format);
        psoDesc.DsvFormat = IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).Format;
        psoDesc.RasterRenderState.DepthClipEnable = false;
       // psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::Front;

        psoDesc.DepthStencilRenderState = {};
        psoDesc.DepthStencilRenderState.DepthWriteEnable = false;
        psoDesc.DepthStencilRenderState.DepthFunc = RHI::ComparisonFunc::LessOrEqual;

        this->m_pso[PSO_Sky] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
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

    // Any Fullscreen Quad shaders
    {
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_FullscreenQuad);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_FullscreenQuad);
        psoDesc.InputLayout = nullptr;
        //psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::Front;
        psoDesc.DepthStencilRenderState.DepthTestEnable = false;
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(IGraphicsDevice::Ptr->GetBackBuffer()).Format);
        this->m_pso[PSO_FullScreenQuad] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);

        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_ToneMapping);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_ToneMapping);

        this->m_pso[PSO_ToneMappingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);

        psoDesc.RtvFormats.clear();
        psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).Format);
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_DeferredLighting);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_DeferredLighting);
        this->m_pso[PSO_DeferredLightingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
    }

    // ENV Map
    {
        RHI::GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_EnvMap_Sky);
        psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_EnvMap_SkyProcedural);
        // TODO handle GS 
        assert(IGraphicsDevice::Ptr->CheckCapability(DeviceCapability::RT_VT_ArrayIndex_Without_GS));

        this->m_pso[PSO_EnvCapture_SkyProcedural] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
    }
}

void DeferredRenderer::RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene)
{
    this->m_commandList->Open();

    this->PrepareFrameRenderData(this->m_commandList, scene);

    Shader::Camera cameraData = {};
    cameraData.CameraPosition = camera.Eye;
    TransposeMatrix(camera.ViewProjection, &cameraData.ViewProjection);
    TransposeMatrix(camera.ViewProjectionInv, &cameraData.ViewProjectionInv);
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

        this->m_commandList->BindConstantBuffer(RootParameters_DeferredLightingFulLQuad::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(RootParameters_DeferredLightingFulLQuad::CameraCB, cameraData);

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
        this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
    }

    {
        auto scrope = this->m_commandList->BeginScopedMarker("Draw Sky Pass");

        this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_Sky]);

        this->m_commandList->BindConstantBuffer(DefaultRootParameters::FrameCB, this->m_constantBuffers[CB_Frame]);
        this->m_commandList->BindDynamicConstantBuffer(DefaultRootParameters::CameraCB, cameraData);

        this->m_commandList->SetRenderTargets({ this->m_deferredLightBuffer }, this->m_depthBuffer);

        this->m_commandList->Draw(3, 1, 0, 0);
    }

	{
        auto _ = this->m_commandList->BeginScopedMarker("Post Process FX");
		{
			auto scrope = this->m_commandList->BeginScopedMarker("Tone Mapping");

			this->m_commandList->SetGraphicsPSO(this->m_pso[PSO_ToneMappingPass]);
            this->m_commandList->SetRenderTargets({ this->m_finalColourBuffer }, TextureHandle());


            RHI::GpuBarrier preTransition[] =
            {
                RHI::GpuBarrier::CreateTexture(
                    this->m_deferredLightBuffer,
                    IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).InitialState,
                    RHI::ResourceStates::ShaderResource),
            };
            this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(preTransition, _countof(preTransition)));

            // Exposure, not needed right now
            this->m_commandList->BindPushConstant(RootParameters_ToneMapping::Push, 1.0f);

            this->m_commandList->BindDynamicDescriptorTable(
                RootParameters_ToneMapping::HDRBuffer,
                {
                    this->m_deferredLightBuffer,
                });

            this->m_commandList->Draw(3, 1, 0, 0);

            RHI::GpuBarrier postTransition[] =
            {
                RHI::GpuBarrier::CreateTexture(
                    this->m_deferredLightBuffer,
                    RHI::ResourceStates::ShaderResource,
                    IGraphicsDevice::Ptr->GetTextureDesc(this->m_deferredLightBuffer).InitialState),
            };
            this->m_commandList->TransitionBarriers(Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
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
