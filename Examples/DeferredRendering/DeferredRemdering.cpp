#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Core/Math.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;

namespace MeshPrefabs
{
    void CreateCube(float size, Scene::Entity materialEntity, Scene::MeshComponent& meshComponent, bool isLHCoord = false)
    {
        // A cube has six faces, each one pointing in a different direction.
        const int FaceCount = 6;

        static const XMVECTORF32 faceNormals[FaceCount] =
        {
            { 0,  0,  1 },
            { 0,  0, -1 },
            { 1,  0,  0 },
            { -1,  0,  0 },
            { 0,  1,  0 },
            { 0, -1,  0 },
        };

        static const XMFLOAT3 faceColour[] =
        {
            { 1.0f,  0.0f,  0.0f },
            { 0.0f,  1.0f,  0.0f },
            { 0.0f,  0.0f,  1.0f },
        };

        static const XMFLOAT2 textureCoordinates[4] =
        {
            { 1, 0 },
            { 1, 1 },
            { 0, 1 },
            { 0, 0 },
        };

        size /= 2;

        for (int i = 0; i < FaceCount; i++)
        {
            XMVECTOR normalV = faceNormals[i];

            // Get two vectors perpendicular both to the face normal and to each other.
            XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

            XMVECTOR side1 = XMVector3Cross(normalV, basis);
            XMVECTOR side2 = XMVector3Cross(normalV, side1);

            // Six indices (two triangles) per face.
            size_t vbase = meshComponent.VertexPositions.size();
            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 0));
            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 1));
            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 2));

            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 0));
            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 2));
            meshComponent.Indices.push_back(static_cast<uint32_t>(vbase + 3));

            XMFLOAT3 positon;
            XMStoreFloat3(&positon, (normalV - side1 - side2) * size);
            meshComponent.VertexPositions.push_back(positon);
            XMStoreFloat3(&positon, (normalV - side1 + side2) * size);
            meshComponent.VertexPositions.push_back(positon);
            XMStoreFloat3(&positon, (normalV + side1 + side2) * size);
            meshComponent.VertexPositions.push_back(positon);
            XMStoreFloat3(&positon, (normalV + side1 - side2) * size);
            meshComponent.VertexPositions.push_back(positon);

            meshComponent.VertexTexCoords.push_back(textureCoordinates[0]);
            meshComponent.VertexTexCoords.push_back(textureCoordinates[1]);
            meshComponent.VertexTexCoords.push_back(textureCoordinates[2]);
            meshComponent.VertexTexCoords.push_back(textureCoordinates[3]);

            meshComponent.VertexColour.push_back(faceColour[0]);
            meshComponent.VertexColour.push_back(faceColour[1]);
            meshComponent.VertexColour.push_back(faceColour[2]);
            meshComponent.VertexColour.push_back(faceColour[3]);

            DirectX::XMFLOAT3 normal;
            DirectX::XMStoreFloat3(&normal, normalV);
            meshComponent.VertexNormals.push_back(normal);
            meshComponent.VertexNormals.push_back(normal);
            meshComponent.VertexNormals.push_back(normal);
            meshComponent.VertexNormals.push_back(normal);

        }

        meshComponent.ComputeTangents();

        if (isLHCoord)
        {
            meshComponent.ReverseWinding();
        }

        meshComponent.Flags = ~0u;

        auto& meshGeometry = meshComponent.Surfaces.emplace_back();
        {
            meshGeometry.Material = materialEntity;
        }

        meshGeometry.IndexOffsetInMesh = 0;
        meshGeometry.VertexOffsetInMesh = 9;
        meshGeometry.NumIndices = meshComponent.Indices.size();
        meshGeometry.NumVertices = meshComponent.VertexPositions.size();

        meshComponent.TotalIndices = meshGeometry.NumIndices;
        meshComponent.TotalVertices = meshGeometry.NumVertices;
    }
}

struct GBufferRenderTargets
{
    RHI::TextureHandle DepthTex;
    RHI::TextureHandle AlbedoTex;
    RHI::TextureHandle NormalTex;
    RHI::TextureHandle SurfaceTex;
    RHI::TextureHandle SpecularTex;

    RHI::RenderPassHandle RenderPass;
    DirectX::XMFLOAT2 CanvasSize;


    // Extra
    RHI::TextureHandle FinalColourBuffer;

    void Initialize(
        RHI::IGraphicsDevice* gfxDevice,
        DirectX::XMFLOAT2 const& size)
    {
        CanvasSize = size;

        // -- Depth ---
        RHI::TextureDesc desc = {};
        desc.Width = std::max(size.x, 1.0f);
        desc.Height = std::max(size.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.IsBindless = false;

        desc.Format = RHI::RHIFormat::D32;
        desc.IsTypeless = true;
        desc.DebugName = "Depth Buffer";
        desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthWrite;
        this->DepthTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);
        // -- Depth end ---

        // -- GBuffers ---
        desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
        desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
        desc.InitialState = RHI::ResourceStates::ShaderResource;
        desc.IsBindless = true;

        desc.Format = RHI::RHIFormat::RGBA32_FLOAT;
        desc.DebugName = "Albedo Buffer";
        this->AlbedoTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        // desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
        desc.Format = RHI::RHIFormat::RGBA16_SNORM;
        desc.DebugName = "Normal Buffer";
        this->NormalTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::RGBA8_UNORM;
        desc.DebugName = "Surface Buffer";
        this->SurfaceTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::SRGBA8_UNORM;
        desc.DebugName = "Specular Buffer";
        this->SpecularTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
        desc.DebugName = "Final Colour Buffer";
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::ShaderResource | BindingFlags::UnorderedAccess;
        this->FinalColourBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        this->RenderPass = IGraphicsDevice::GPtr->CreateRenderPass(
            {
                .Attachments =
                {
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->AlbedoTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->NormalTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->SurfaceTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->SpecularTex,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                    {
                        .Type = RenderPassAttachment::Type::DepthStencil,
                        .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                        .Texture = this->DepthTex,
                        .InitialLayout = RHI::ResourceStates::DepthWrite,
                        .SubpassLayout = RHI::ResourceStates::DepthWrite,
                        .FinalLayout = RHI::ResourceStates::DepthWrite
                    },
                }
            });
    }

    void Free(RHI::IGraphicsDevice* gfxDevice)
    {
        gfxDevice->DeleteRenderPass(this->RenderPass);
        gfxDevice->DeleteTexture(this->DepthTex);
        gfxDevice->DeleteTexture(this->AlbedoTex);
        gfxDevice->DeleteTexture(this->NormalTex);
        gfxDevice->DeleteTexture(this->SurfaceTex);
        gfxDevice->DeleteTexture(this->SpecularTex);
    }
};


struct DeferredLightingPass
{
    struct Input
    {
        RHI::TextureHandle Depth;
        RHI::TextureHandle GBufferAlbedo;
        RHI::TextureHandle GBufferNormals;
        RHI::TextureHandle GBufferSurface;
        RHI::TextureHandle GBufferSpecular;

        RHI::TextureHandle OutputTexture;

        RHI::BufferHandle FrameBuffer;
        Shader::Camera* CameraData;

        void FillGBuffer(GBufferRenderTargets const& renderTargets)
        {
            this->Depth = renderTargets.DepthTex;
            this->GBufferAlbedo = renderTargets.AlbedoTex;
            this->GBufferNormals = renderTargets.NormalTex;
            this->GBufferSurface = renderTargets.SurfaceTex;
            this->GBufferSpecular = renderTargets.SpecularTex;
        }
    };

    DeferredLightingPass(IGraphicsDevice* gfxDevice, std::shared_ptr<Renderer::CommonPasses> commonPasses)
        : m_gfxDevice(gfxDevice)
        , m_commonPasses(commonPasses)
    {
    }

    void Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_pixelShader = factory.CreateShader(
            "PhxEngine/DeferredLightingPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "DeferredLightingPS",
            });

        this->m_computeShader = factory.CreateShader(
            "PhxEngine/DeferredLightingCS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "DeferredLightingCS",
            });
    }

    void Render(ICommandList* commandList, Input const& input)
    {
        // Set Push Constants for input texture Data.
        if (!this->m_computePso.IsValid())
        {
            this->m_computePso = this->m_gfxDevice->CreateComputePipeline({
                    .ComputeShader = this->m_computeShader
                });
        }

        auto _ = commandList->BeginScopedMarker("Deferred Lighting Pass CS");

        RHI::GpuBarrier preBarriers[] =
        {
            RHI::GpuBarrier::CreateTexture(input.Depth, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateTexture(input.OutputTexture, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState, RHI::ResourceStates::UnorderedAccess),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

        // TODO: Change to match same syntax as GFX pipeline
        commandList->SetComputeState(this->m_computePso);

        commandList->BindConstantBuffer(1, input.FrameBuffer);
        commandList->BindDynamicConstantBuffer(2, *input.CameraData);
        commandList->BindDynamicDescriptorTable(
            4,
            {
                input.Depth,
                input.GBufferAlbedo,
                input.GBufferNormals,
                input.GBufferSurface,
                input.GBufferSpecular
            });
        commandList->BindDynamicUavDescriptorTable(5, { input.OutputTexture });

        auto& outputDesc = this->m_gfxDevice->GetTextureDesc(input.OutputTexture);

        Shader::DefferedLightingCSConstants push = {};
        push.DipatchGridDim =
        {
            outputDesc.Width / DEFERRED_BLOCK_SIZE_X,
            outputDesc.Height / DEFERRED_BLOCK_SIZE_Y,
        };
        push.MaxTileWidth = 16;

        commandList->Dispatch(
            push.DipatchGridDim.x,
            push.DipatchGridDim.y,
            1);

        RHI::GpuBarrier postTransition[] =
        {
            RHI::GpuBarrier::CreateTexture(input.Depth, RHI::ResourceStates::ShaderResource, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState),
            RHI::GpuBarrier::CreateTexture(input.OutputTexture, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
    }

private:
    IGraphicsDevice* m_gfxDevice;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
    RHI::ShaderHandle m_pixelShader;
    RHI::ShaderHandle m_computeShader;
    RHI::ComputePipelineHandle m_computePso;
};

struct GBufferFillPass
{
    void Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_vertexShader = factory.CreateShader(
            "PhxEngine/GBufferPassVS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "GBufferPassVS",
            });

        this->m_pixelShader = factory.CreateShader(
            "PhxEngine/GBufferPassPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "GBufferPassPS",
            });
    }

    void WindowResized(IGraphicsDevice* gfxDevice)
    {
        gfxDevice->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void BeginPass(IGraphicsDevice* gfxDevice, ICommandList* commandList, BufferHandle frameCB, Shader::Camera cameraData, GBufferRenderTargets const& gbufferRenderTargets)
    {
		commandList->BeginRenderPass(gbufferRenderTargets.RenderPass);

		if (!this->m_pipeline.IsValid())
		{
			this->m_pipeline = gfxDevice->CreateGraphicsPipeline(
				{
					.VertexShader = this->m_vertexShader,
					.PixelShader = this->m_pixelShader,
			        .RtvFormats = {
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.AlbedoTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.NormalTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SurfaceTex).Format,
				        IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SpecularTex).Format,
			        },
			        .DsvFormat = { IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.DepthTex).Format }
				});
		}
        commandList->SetGraphicsPipeline(this->m_pipeline);

        RHI::Viewport v(gbufferRenderTargets.CanvasSize.x, gbufferRenderTargets.CanvasSize.y);
        commandList->SetViewports(&v, 1);

        RHI::Rect rec(LONG_MAX, LONG_MAX);
        commandList->SetScissors(&rec, 1);

        commandList->BindConstantBuffer(1, frameCB);

        // TODO: Create a camera const buffer as well
        commandList->BindDynamicConstantBuffer(2, cameraData);
    }

    void EndPass(ICommandList* commandList)
    {
        commandList->EndRenderPass();
    }
private:
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::GraphicsPipelineHandle m_pipeline;

};

class DeferredRendering : public EngineRenderPass
{
private:

public:
    DeferredRendering(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        // Load Data in seperate thread?
        // Create File Sustem
        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();

        std::filesystem::path appShadersRoot = Core::Platform::GetExcecutableDir() / "Shaders/PhxEngine/dxil";
        std::shared_ptr<Core::IRootFileSystem> rootFilePath = Core::CreateRootFileSystem();
        rootFilePath->Mount("/Shaders/PhxEngine", appShadersRoot);

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");

        auto textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());
        this->CreateSimpleScene(textureCache.get());

        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_gbufferFillPass.Initialize(*this->m_shaderFactory);
        this->m_deferredLightingPass = std::make_shared<DeferredLightingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_deferredLightingPass->Initialize(*this->m_shaderFactory);

        this->m_frameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });


        DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(0.0f, 0.0f, -3.5f, 0.0f);
        DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        DirectX::XMVECTOR eyeDir = DirectX::XMVectorSubtract(focusPoint, eyePos);
        eyeDir = DirectX::XMVector3Normalize(eyeDir);

        DirectX::XMStoreFloat3(
            &this->m_mainCamera.Eye,
            eyePos);

        DirectX::XMStoreFloat3(
            &this->m_mainCamera.Forward,
            eyeDir);

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);
        this->m_mainCamera.UpdateCamera();
        return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
        this->m_renderTargets.Free(this->GetGfxDevice());
        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_gbufferFillPass.WindowResized(this->GetGfxDevice());
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->m_rotation += deltaTime.GetSeconds() * 2.5f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});

    }

    void Render() override
    {
        this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
        this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
        this->m_mainCamera.UpdateCamera();

        auto& cubeTransform = this->m_cubeInstance.GetComponent<Scene::TransformComponent>();
        cubeTransform.LocalRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        cubeTransform.MatrixTransform(DirectX::XMMatrixRotationRollPitchYaw(0.0f, DirectX::XMConvertToRadians(this->m_rotation), 0.0f));
        // cubeTransform.MatrixTransform(DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(-30), 0.0f, 0.0f));
        cubeTransform.UpdateTransform();

        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        {
            auto _ = commandList->BeginScopedMarker("Preare Frame Data");
            this->PrepareRenderData(commandList, this->m_simpleScene);
        }

        // The camera data doesn't match whats in the CPU and what's in the GPU.
        Shader::Camera cameraData = {};
        cameraData.CameraPosition = this->m_mainCamera.Eye;
        cameraData.ViewProjection = this->m_mainCamera.ViewProjection;
        cameraData.ViewProjectionInv = this->m_mainCamera.ViewProjectionInv;
        cameraData.ProjInv = this->m_mainCamera.ProjectionInv;
        cameraData.ViewInv = this->m_mainCamera.ViewInv;

        // Set up RenderData
        {

            auto _ = commandList->BeginScopedMarker("GBuffer Fill");

            this->m_gbufferFillPass.BeginPass(this->GetGfxDevice(), commandList, this->m_frameConstantBuffer, cameraData, this->m_renderTargets);
            auto view = this->m_simpleScene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::TransformComponent>();
            uint32_t instanceCount = 0;
            for (auto e : view)
            {
                instanceCount++;
            }

            GPUAllocation instanceBufferAlloc =
                commandList->AllocateGpu(
                    sizeof(Shader::ShaderMeshInstancePointer) * (size_t)instanceCount,
                    sizeof(Shader::ShaderMeshInstancePointer));

            const DescriptorIndex instanceBufferDescriptorIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(instanceBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
            Shader::ShaderMeshInstancePointer* pInstancePointerData = (Shader::ShaderMeshInstancePointer*)instanceBufferAlloc.CpuData;
            instanceCount = 0;
            for (auto e : view)
            {
                auto [meshInstance, transform] = view.get<Scene::MeshInstanceComponent, Scene::TransformComponent>(e);
                Shader::ShaderMeshInstancePointer shaderMeshPtr = {};
                shaderMeshPtr.Create(meshInstance.GlobalBufferIndex, 0);

                // Write into actual GPU-buffer:
                std::memcpy(pInstancePointerData + instanceCount, &shaderMeshPtr, sizeof(shaderMeshPtr)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
                instanceCount++;

                auto meshComponent= this->m_simpleScene.GetRegistry().get<Scene::MeshComponent>(meshInstance.Mesh);
                commandList->BindIndexBuffer(meshComponent.IndexGpuBuffer);

                for (size_t i = 0; i < meshComponent.Surfaces.size(); i++)
                {
                    auto& materiaComp = this->m_simpleScene.GetRegistry().get<Scene::MaterialComponent>(meshComponent.Surfaces[i].Material);

                    Shader::GeometryPassPushConstants pushConstant = {};
                    pushConstant.GeometryIndex = meshComponent.GlobalGeometryBufferIndex + i;
                    pushConstant.MaterialIndex = materiaComp.GlobalBufferIndex;
                    pushConstant.InstancePtrBufferDescriptorIndex = instanceBufferDescriptorIndex;
                    pushConstant.InstancePtrDataOffset = (uint32_t)(instanceBufferAlloc.Offset + instanceCount * sizeof(Shader::ShaderMeshInstancePointer));

                    commandList->BindPushConstant(0, pushConstant);

                    commandList->DrawIndexed(
                        meshComponent.Surfaces[i].NumIndices,
                        1,
                        meshComponent.Surfaces[i].IndexOffsetInMesh);
                }
            }

            this->m_gbufferFillPass.EndPass(commandList);
        }

		DeferredLightingPass::Input input = {};
		input.FillGBuffer(this->m_renderTargets);
		input.OutputTexture = this->m_renderTargets.FinalColourBuffer;
        input.FrameBuffer = this->m_frameConstantBuffer;
        input.CameraData = &cameraData;

		this->m_deferredLightingPass->Render(commandList, input);

        this->m_commonPasses->BlitTexture(
            commandList,
            this->m_renderTargets.FinalColourBuffer,
            commandList->GetRenderPassBackBuffer(),
            this->GetRoot()->GetCanvasSize());

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

private:
    void CreateSimpleScene(Graphics::TextureCache* textureCache)
    {
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        Scene::Entity materialEntity = this->m_simpleScene.CreateEntity("Material");
        auto& mtl = materialEntity.AddComponent<Scene::MaterialComponent>();
        mtl.Roughness = 0.4;
        mtl.Ao = 0.4;
        mtl.Metalness = 0.4;
        mtl.BaseColour = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

        std::filesystem::path texturePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Textures/TestTexture.png";
        mtl.BaseColourTexture = textureCache->LoadTexture(texturePath, true, commandList);

        Scene::Entity meshEntity = this->m_simpleScene.CreateEntity("Cube");
        auto& mesh = meshEntity.AddComponent<Scene::MeshComponent>();
       
        MeshPrefabs::CreateCube(1, materialEntity, mesh, true);

        // Add a Mesh Instance
        this->m_cubeInstance = this->m_simpleScene.CreateEntity("Cube Instance");
        auto& meshInstance = this->m_cubeInstance.AddComponent<Scene::MeshInstanceComponent>();
        meshInstance.Mesh = meshEntity;

        Renderer::ResourceUpload indexUpload;
        Renderer::ResourceUpload vertexUpload;
        this->m_simpleScene.ConstructRenderData(commandList, indexUpload, vertexUpload);
        indexUpload.Free();
        vertexUpload.Free();

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);
    }

    void PrepareRenderData(ICommandList* commandList, Scene::Scene& scene)
    {
        scene.OnUpdate();

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

        Shader::ShaderLight* renderLight = lightArray;
        renderLight->SetType(Scene::LightComponent::kDirectionalLight);
        renderLight->SetRange(10.0f);
        renderLight->SetIntensity(10.0f);
        renderLight->SetFlags(Scene::LightComponent::kEnabled);
        renderLight->SetDirection({ 0.1, -1.0, 0.2 });
        renderLight->ColorPacked = Core::Math::PackColour({ 1.0f, 1.0f, 1.0f, 1.0f });
        renderLight->SetIndices(0);
        renderLight->SetNumCascades(0);
        renderLight->Position = { 0.0f, 0.0f, 0.0f };

        Shader::Frame frameData = {};
        // Move to Renderer...
        frameData.BrdfLUTTexIndex = scene.GetBrdfLutDescriptorIndex();
        frameData.LightEntityDescritporIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(lightBufferAlloc.GpuBuffer, RHI::SubresouceType::SRV);
        frameData.LightDataOffset = lightBufferAlloc.Offset;
        frameData.LightCount = 1;

        frameData.MatricesDescritporIndex = RHI::cInvalidDescriptorIndex;
        frameData.MatricesDataOffset = 0;
        frameData.SceneData = scene.GetShaderData();

        frameData.SceneData.AtmosphereData.AmbientColour = float3(0.4f, 0.0f, 0.4f);

        // Upload data
        RHI::GpuBarrier preCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::ConstantBuffer, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        };
        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

        commandList->WriteBuffer(this->m_frameConstantBuffer, frameData);

        commandList->CopyBuffer(
            scene.GetInstanceBuffer(),
            0,
            scene.GetInstanceUploadBuffer(),
            0,
            scene.GetNumInstances() * sizeof(Shader::MeshInstance));

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
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ConstantBuffer),
            RHI::GpuBarrier::CreateBuffer(scene.GetInstanceBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(scene.GetGeometryBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(scene.GetMaterialBuffer(), RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
    }

private:
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::GraphicsPipelineHandle m_pipeline;
    Scene::Scene m_simpleScene;
    Scene::CameraComponent m_mainCamera;
    Scene::Entity m_cubeInstance;

    RHI::BufferHandle m_frameConstantBuffer;

    GBufferRenderTargets m_renderTargets;
    GBufferFillPass m_gbufferFillPass;
    std::shared_ptr<DeferredLightingPass> m_deferredLightingPass;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;

    float m_rotation = 0.0f;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Basic Triangle";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        DeferredRendering example(root.get());
        if (example.Initialize())
        {
            root->AddPassToBack(&example);
            root->Run();
            root->RemovePass(&example);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    return 0;
}
