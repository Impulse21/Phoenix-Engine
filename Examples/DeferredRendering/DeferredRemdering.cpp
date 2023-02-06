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

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;

namespace CubeGeometry
{
    static constexpr uint16_t kIndices[] = {
         0,  1,  2,   0,  3,  1, // front face
         4,  5,  6,   4,  7,  5, // left face
         8,  9, 10,   8, 11,  9, // right face
        12, 13, 14,  12, 15, 13, // back face
        16, 17, 18,  16, 19, 17, // top face
        20, 21, 22,  20, 23, 21, // bottom face
    };

    static constexpr DirectX::XMFLOAT3 kPositions[] = {
        {-0.5f,  0.5f, -0.5f}, // front face
        { 0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},

        { 0.5f, -0.5f, -0.5f}, // right side face
        { 0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f, -0.5f},

        {-0.5f,  0.5f,  0.5f}, // left side face
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f, -0.5f},

        { 0.5f,  0.5f,  0.5f}, // back face
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},

        {-0.5f,  0.5f, -0.5f}, // top face
        { 0.5f,  0.5f,  0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f,  0.5f},

        { 0.5f, -0.5f,  0.5f}, // bottom face
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
    };

    static constexpr DirectX::XMFLOAT2 kTexCoords[] = {
        {0.0f, 0.0f}, // front face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 1.0f}, // right side face
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f}, // left side face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 0.0f}, // back face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 1.0f}, // top face
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},

        {1.0f, 1.0f}, // bottom face
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    static constexpr DirectX::XMFLOAT3 kNormals[] = {
        { 0.0f, 0.0f, -1.0f }, // front face
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f },

        { 1.0f, 0.0f, 0.0f }, // right side face
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },

        { -1.0f, 0.0f, 0.0f }, // left side face
        { -1.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },

        { 0.0f, 0.0f, 1.0f }, // back face
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },

        { 0.0f, 1.0f, 0.0f }, // top face
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },

        { 0.0f, -1.0f, 0.0f }, // bottom face
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
    };

    static constexpr DirectX::XMFLOAT4 kTangents[] = {
        { 1.0f, 0.0f, 0.0f, 1.0f }, // front face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
          
        { 0.0f, 0.0f, 1.0f, 1.0f }, // right side face
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
          
        { 0.0f, 0.0f, -1.0f, 1.0f }, // left side face
        { 0.0f, 0.0f, -1.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f, 1.0f },
          
        { -1.0f, 0.0f, 0.0f, 1.0f }, // back face
        { -1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f, 1.0f },
          
        { 1.0f, 0.0f, 0.0f, 1.0f }, // top face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
          
        { 1.0f, 0.0f, 0.0f, 1.0f }, // bottom face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
    };
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
        desc.OptmizedClearValue.DepthStencil.Depth = 0.0f;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthRead;
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
                        .InitialLayout = RHI::ResourceStates::DepthRead,
                        .SubpassLayout = RHI::ResourceStates::DepthWrite,
                        .FinalLayout = RHI::ResourceStates::DepthRead
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
			        .DepthStencilRenderState = {.DepthFunc = ComparisonFunc::Greater },
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

        this->m_frameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });

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
        this->m_rotation += deltaTime.GetSeconds() * 1.1f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});

    }

    void Render() override
    {
        Scene::TransformComponent t = {};
        t.RotateRollPitchYaw({ DirectX::XMConvertToRadians(this->m_rotation), 0.0f, 0.0f });
        t.RotateRollPitchYaw({ 0.0f, DirectX::XMConvertToRadians(-30.f), 0.0f });
        t.LocalTranslation = DirectX::XMFLOAT3(0.0f, 0.0f, -2.0f);
        t.UpdateTransform();

        this->m_mainCamera.TransformCamera(t);
        this->m_mainCamera.UpdateCamera();

        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        this->PrepareRenderData(commandList, this->m_simpleScene);

        // Set up RenderData
        {
            Shader::Camera cameraData = {};
            cameraData.CameraPosition = this->m_mainCamera.Eye;
            cameraData.ViewProjection = this->m_mainCamera.ViewProjection;
            cameraData.ViewProjectionInv = this->m_mainCamera.ViewProjectionInv;
            cameraData.ProjInv = this->m_mainCamera.ProjectionInv;
            cameraData.ViewInv = this->m_mainCamera.ViewInv;

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

        this->m_commonPasses->BlitTexture(
            commandList,
            this->m_renderTargets.AlbedoTex,
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
        
        mesh.Indices.resize(ARRAYSIZE(CubeGeometry::kIndices));
        std::memcpy(mesh.Indices.data(), CubeGeometry::kIndices, sizeof(CubeGeometry::kIndices[0]) * ARRAYSIZE(CubeGeometry::kIndices));

        mesh.VertexPositions.resize(ARRAYSIZE(CubeGeometry::kPositions));
        std::memcpy(mesh.VertexPositions.data(), CubeGeometry::kPositions, sizeof(CubeGeometry::kPositions[0]) * ARRAYSIZE(CubeGeometry::kPositions));

        mesh.VertexNormals.resize(ARRAYSIZE(CubeGeometry::kNormals));
        std::memcpy(mesh.VertexNormals.data(), CubeGeometry::kNormals, sizeof(CubeGeometry::kNormals[0]) * ARRAYSIZE(CubeGeometry::kNormals));

        mesh.VertexTangents.resize(ARRAYSIZE(CubeGeometry::kTangents));
        std::memcpy(mesh.VertexTangents.data(), CubeGeometry::kTangents, sizeof(CubeGeometry::kTangents[0]) * ARRAYSIZE(CubeGeometry::kTangents));

        mesh.VertexTexCoords.resize(ARRAYSIZE(CubeGeometry::kTexCoords));
        std::memcpy(mesh.VertexTexCoords.data(), CubeGeometry::kTexCoords, sizeof(CubeGeometry::kTexCoords[0]) * ARRAYSIZE(CubeGeometry::kTexCoords));

        mesh.VertexPositions.resize(ARRAYSIZE(CubeGeometry::kPositions));
        std::memcpy(mesh.VertexPositions.data(), CubeGeometry::kPositions, sizeof(CubeGeometry::kPositions[0]) * ARRAYSIZE(CubeGeometry::kPositions));

        mesh.Flags = ~0u;

        auto& meshGeometry = mesh.Surfaces.emplace_back();
        {
            meshGeometry.Material = materialEntity;
        }

        meshGeometry.IndexOffsetInMesh = 0;
        meshGeometry.VertexOffsetInMesh = 9;
        meshGeometry.NumIndices = ARRAYSIZE(CubeGeometry::kIndices);
        meshGeometry.NumVertices = ARRAYSIZE(CubeGeometry::kPositions);

        mesh.TotalIndices = meshGeometry.NumIndices;
        mesh.TotalVertices = meshGeometry.NumVertices;

        // Add a Mesh Instance
        Scene::Entity meshInstanceEntity = this->m_simpleScene.CreateEntity("Cube Instance");
        auto& meshInstance = meshInstanceEntity.AddComponent<Scene::MeshInstanceComponent>();
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
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
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
            RHI::GpuBarrier::CreateBuffer(this->m_frameConstantBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
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
    RHI::BufferHandle m_frameConstantBuffer;
    GBufferRenderTargets m_renderTargets;
    GBufferFillPass m_gbufferFillPass;
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
