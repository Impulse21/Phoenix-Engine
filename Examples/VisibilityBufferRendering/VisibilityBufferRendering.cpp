#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/SceneLoader.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Renderer/VisibilityBufferPasses.h>
#include <PhxEngine/Renderer/DeferredLightingPass.h>
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Renderer/GBuffer.h>
#include <PhxEngine/Engine/CameraControllers.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

struct RenderTargets
{
    // Extra
    RHI::TextureHandle VisibilityBuffer;
    RHI::RenderPassHandle VisibilityRenderPass;
    RHI::TextureHandle ColourBuffer;

    void Initialize(
        RHI::IGraphicsDevice* gfxDevice,
        DirectX::XMFLOAT2 const& size)
    {
        RHI::TextureDesc desc = {};
        desc.Width = std::max(size.x, 1.0f);
        desc.Height = std::max(size.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.OptmizedClearValue.Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
        desc.InitialState = RHI::ResourceStates::ShaderResource;
        desc.IsBindless = true;
        desc.IsTypeless = true;
        desc.Format = RHI::RHIFormat::RGBA16_FLOAT;
        desc.DebugName = "Colour Buffer";
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | BindingFlags::UnorderedAccess;

        this->ColourBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        RHI::TextureDesc visibilityBufferDesc = {};
        visibilityBufferDesc.Width = std::max(size.x, 1.0f);
        visibilityBufferDesc.Height = std::max(size.y, 1.0f);
        visibilityBufferDesc.Dimension = RHI::TextureDimension::Texture2D;
        visibilityBufferDesc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
        visibilityBufferDesc.InitialState = RHI::ResourceStates::ShaderResource;
        visibilityBufferDesc.IsBindless = true;
        visibilityBufferDesc.IsTypeless = true;
        visibilityBufferDesc.Format = RHI::RHIFormat::RGBA8_UNORM;
        visibilityBufferDesc.DebugName = "Visibility Buffer";
        visibilityBufferDesc.BindingFlags = RHI::BindingFlags::ShaderResource;

        this->VisibilityBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(visibilityBufferDesc);
        this->VisibilityRenderPass = IGraphicsDevice::GPtr->CreateRenderPass(
            {
                .Attachments =
                {
                    {
                        .LoadOp = RenderPassAttachment::LoadOpType::Load,
                        .Texture = this->VisibilityBuffer,
                        .InitialLayout = RHI::ResourceStates::ShaderResource,
                        .SubpassLayout = RHI::ResourceStates::RenderTarget,
                        .FinalLayout = RHI::ResourceStates::ShaderResource
                    },
                }
            });
    }

    void Free(RHI::IGraphicsDevice* gfxDevice)
    {
        gfxDevice->DeleteRenderPass(this->VisibilityRenderPass);
        gfxDevice->DeleteTexture(this->VisibilityBuffer);
        gfxDevice->DeleteTexture(this->ColourBuffer);
    }
};

struct SceneVisibilityRenderer3D
{
    SceneVisibilityRenderer3D(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<PhxEngine::Renderer::CommonPasses> commonPasses)
    {

    }

    void Initialize(Graphics::ShaderFactory& factory)
    {

    }
private:
    RHI::RenderPassHandle m_visFillRenderPass;
    RHI::TextureHandle m_visibilityBuffer;
    RHI::TextureHandle m_depthBuffer;

    RHI::RenderPassHandle m_lightingRenderPass;
    RHI::TextureHandle m_colourBuffer;

    RHI::ShaderHandle m_cullPassCS;

    RHI::ShaderHandle m_visFillPassVS;
    RHI::ShaderHandle m_visFillPassMS;
    RHI::ShaderHandle m_visFillPassPS;

    RHI::ShaderHandle m_visDeferredLightPassCS;

    RHI::GraphicsPipelineHandle m_pipelineVisFillGfx;
    RHI::MeshPipelineHandle m_pipelineVisFillMesh;

    RHI::ComputePipelineHandle m_pipelineCullPipeline;
    RHI::ComputePipelineHandle m_pipelineVisDeferredPass;

};

struct RenderScene
{
    RHI::BufferHandle GlobalVertexBuffer;
    RHI::BufferHandle GlobalIndexBuffer;

    void MergeMeshes(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
    {
        uint64_t vertexCount = 0;
        uint64_t vertexSizeInBytes = 0;
        uint64_t indexCount = 0;
        uint64_t indexSizeInBytes = 0;

        auto view = scene.GetAllEntitiesWith<Scene::MeshComponent>();
        for (auto e : view)
        {
            auto& meshComp = view.get<Scene::MeshComponent>(e);

            meshComp.GlobalIndexBufferOffset = indexCount;
            indexCount += meshComp.TotalIndices;
            indexSizeInBytes += meshComp.GetIndexBufferSizeInBytes();

            meshComp.GlobalVertexBufferOffset = vertexCount;
            vertexCount += meshComp.TotalVertices;
            vertexSizeInBytes += meshComp.GetVertexBufferSizeInBytes();
        }

        if (this->GlobalVertexBuffer.IsValid())
        {
            gfxDevice->DeleteBuffer(this->GlobalVertexBuffer);
        }

        RHI::BufferDesc vertexDesc = {};
        vertexDesc.StrideInBytes = sizeof(float);
        vertexDesc.DebugName = "Vertex Buffer";
        vertexDesc.EnableBindless();
        vertexDesc.IsRawBuffer();
        vertexDesc.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource;
        vertexDesc.SizeInBytes = vertexSizeInBytes;

        this->GlobalVertexBuffer = RHI::IGraphicsDevice::GPtr->CreateVertexBuffer(vertexDesc);

        if (this->GlobalIndexBuffer.IsValid())
        {
            gfxDevice->DeleteBuffer(this->GlobalIndexBuffer);
        }

        RHI::BufferDesc indexBufferDesc = {};
        indexBufferDesc.SizeInBytes = indexSizeInBytes;
        indexBufferDesc.StrideInBytes = sizeof(uint32_t);
        indexBufferDesc.DebugName = "Index Buffer";
        vertexDesc.Binding = RHI::BindingFlags::IndexBuffer;
        this->GlobalIndexBuffer = RHI::IGraphicsDevice::GPtr->CreateIndexBuffer(indexBufferDesc);

        // Upload data
        {
            RHI::GpuBarrier preCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(this->GlobalVertexBuffer, gfxDevice->GetBufferDesc(this->GlobalVertexBuffer).InitialState, RHI::ResourceStates::CopyDest),
                RHI::GpuBarrier::CreateBuffer(this->GlobalIndexBuffer, gfxDevice->GetBufferDesc(this->GlobalIndexBuffer).InitialState, RHI::ResourceStates::CopyDest),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));
        }

        for (auto e : view)
        {
            auto& meshComp = view.get<Scene::MeshComponent>(e);;
            
            BufferDesc vertexBufferDesc = gfxDevice->GetBufferDesc(meshComp.VertexGpuBuffer);
            BufferDesc indexBufferDesc = gfxDevice->GetBufferDesc(meshComp.IndexGpuBuffer);
            RHI::GpuBarrier preCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(meshComp.VertexGpuBuffer, vertexBufferDesc.InitialState, RHI::ResourceStates::CopySource),
                RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, indexBufferDesc.InitialState, RHI::ResourceStates::CopySource),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

            commandList->CopyBuffer(
                meshComp.VertexGpuBuffer,
                0,
                this->GlobalVertexBuffer,
                meshComp.GlobalVertexBufferOffset* vertexBufferDesc.StrideInBytes,
                vertexBufferDesc.SizeInBytes);

            commandList->CopyBuffer(
                meshComp.IndexGpuBuffer,
                0,
                this->GlobalIndexBuffer,
                meshComp.GlobalIndexBufferOffset * indexBufferDesc.StrideInBytes,
                indexBufferDesc.SizeInBytes);

            // Upload data
            RHI::GpuBarrier postCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(meshComp.VertexGpuBuffer, RHI::ResourceStates::CopySource, vertexBufferDesc.InitialState),
                RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, RHI::ResourceStates::CopySource, indexBufferDesc.InitialState),
            };

            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
        }


        // Upload data
        RHI::GpuBarrier postCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->GlobalVertexBuffer, RHI::ResourceStates::CopyDest, gfxDevice->GetBufferDesc(this->GlobalVertexBuffer).InitialState),
            RHI::GpuBarrier::CreateBuffer(this->GlobalIndexBuffer, RHI::ResourceStates::CopyDest, gfxDevice->GetBufferDesc(this->GlobalIndexBuffer).InitialState),
        };

        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
    }
};

class VisibilityRenderering : public ApplicationBase
{
private:

public:
    VisibilityRenderering(IPhxEngineRoot* root)
        : ApplicationBase(root)
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
        rootFilePath->Mount("/Shaders/VisibilityBufferRendering", appShadersRoot);

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());

        this->m_meshShader = this->m_shaderFactory->CreateShader(
            "VisibilityBufferRendering/VisibilityBufferFillPassMS.hlsl",
            {
                .Stage = RHI::ShaderStage::Mesh,
                .DebugName = "VisibilityBufferFillPassMS",
            });

        this->m_pixelShader = this->m_shaderFactory->CreateShader(
            "VisibilityBufferRendering/VisibilityBufferFillPassPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "VisibilityBufferFillPassPS",
            });

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/TestScenes/VisibilityBufferScene.gltf";
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());

        this->m_visibilityBufferFillPass = std::make_unique<VisibilityBufferFillPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_visibilityBufferFillPass->Initialize(*this->m_shaderFactory);
        this->m_meshletBufferFillPass = std::make_unique<MeshletBufferFillPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_meshletBufferFillPass->Initialize(*this->m_shaderFactory);

        this->m_deferredLightingPass = std::make_shared<DeferredLightingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_deferredLightingPass->Initialize(*this->m_shaderFactory);

        this->m_toneMappingPass = std::make_unique<Renderer::ToneMappingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_toneMappingPass->Initialize(*this->m_shaderFactory);

        this->m_frameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);

        // -- Depth ---
        RHI::TextureDesc desc = {};
        desc.Width = std::max(this->GetRoot()->GetCanvasSize().x, 1.0f);
        desc.Height = std::max(this->GetRoot()->GetCanvasSize().y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.IsBindless = false;

        desc.Format = RHI::RHIFormat::D32;
        desc.IsTypeless = true;
        desc.DebugName = "Depth Buffer";
        desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthWrite;
        this->m_depthTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

        Scene::TransformComponent t = {};
        t.LocalTranslation = { -5.0f, 2.0f, 0.0f };
        t.RotateRollPitchYaw({ 0.0f, DirectX::XMConvertToRadians(90), 0.0f });
        t.UpdateTransform();

        this->m_mainCamera.TransformCamera(t);
        this->m_mainCamera.UpdateCamera();

        return true;
    }

    bool LoadScene(std::shared_ptr< Core::IFileSystem> fileSystem, std::filesystem::path sceneFilename) override
    {
        std::unique_ptr<Scene::ISceneLoader> sceneLoader = PhxEngine::Scene::CreateGltfSceneLoader();
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        bool retVal = sceneLoader->LoadScene(
            fileSystem,
            this->m_textureCache,
            sceneFilename,
            commandList,
            this->m_scene);

        if (retVal)
        {
            Renderer::ResourceUpload indexUpload;
            Renderer::ResourceUpload vertexUpload;
            this->m_scene.ConstructRenderData(commandList, indexUpload, vertexUpload);


            auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshComponent>();
            for (auto e : view)
            {
                view.get<Scene::MeshComponent>(e).ComputeMeshlets(this->GetGfxDevice(), commandList);
            }

            indexUpload.Free();
            vertexUpload.Free();
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteMeshPipeline(this->m_pipeline);
        this->m_pipeline = {};
        this->m_renderTargets.Free(this->GetGfxDevice());
        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_visibilityBufferFillPass->WindowResized();
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->m_rotation += deltaTime.GetSeconds() * 1.1f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});
        this->m_cameraController.OnUpdate(this->GetRoot()->GetWindow(), deltaTime, this->m_mainCamera);
    }

    void RenderScene() override
    {
		this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
		this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
		this->m_mainCamera.UpdateCamera();

		// The camera data doesn't match whats in the CPU and what's in the GPU.
		Shader::Camera cameraData = {};
		cameraData.ViewProjection = this->m_mainCamera.ViewProjection;
		cameraData.ViewProjectionInv = this->m_mainCamera.ViewProjectionInv;
		cameraData.ProjInv = this->m_mainCamera.ProjectionInv;
		cameraData.ViewInv = this->m_mainCamera.ViewInv;

		if (!this->m_pipeline.IsValid())
		{
			this->m_pipeline = this->GetGfxDevice()->CreateMeshPipeline(
				{
					.MeshShader = this->m_meshShader,
					.PixelShader = this->m_pixelShader,
					.RtvFormats = { this->GetGfxDevice()->GetTextureDesc(this->GetGfxDevice()->GetBackBuffer()).Format },
                    .DsvFormat = { this->GetGfxDevice()->GetTextureDesc(this->m_depthTex).Format }
				});
		}

		RHI::ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();
		{
            this->PrepareRenderData(commandList, this->m_scene);

			auto _ = commandList->BeginScopedMarker("Render Triagnle");
			commandList->BeginRenderPassBackBuffer();

			commandList->SetMeshPipeline(this->m_pipeline);
			auto canvas = this->GetRoot()->GetCanvasSize();
			RHI::Viewport v(canvas.x, canvas.y);
			commandList->SetViewports(&v, 1);

			RHI::Rect rec(LONG_MAX, LONG_MAX);
			commandList->SetScissors(&rec, 1);

            commandList->BindConstantBuffer(1, this->m_frameConstantBuffer);
            commandList->BindDynamicConstantBuffer(2, cameraData);

            auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::AABBComponent>();
            for (auto e : view)
            {
                auto [instanceComponent, aabbComp] = view.get<Scene::MeshInstanceComponent, Scene::AABBComponent>(e);

                auto& meshComponent = this->m_scene.GetRegistry().get<Scene::MeshComponent>(instanceComponent.Mesh);

                Shader::MeshletPushConstants pushConstant = {};
                pushConstant.WorldMatrix = instanceComponent.WorldMatrix;
                pushConstant.VerticesBufferIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.VertexGpuBuffer, RHI::SubresouceType::SRV);
                pushConstant.MeshletsBufferIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.Meshlets, RHI::SubresouceType::SRV);
                pushConstant.PrimitiveIndicesIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.PrimitiveIndices, RHI::SubresouceType::SRV);
                pushConstant.UniqueVertexIBIdx = this->GetGfxDevice()->GetDescriptorIndex(meshComponent.UniqueVertexIndices, RHI::SubresouceType::SRV);
                pushConstant.GeometryIdx = meshComponent.Surfaces.front().GlobalGeometryBufferIndex;


                for (auto& subset : meshComponent.MeshletSubsets)
                {
                    pushConstant.SubsetOffset = subset.Offset;
                    commandList->BindPushConstant(0, pushConstant);
                    commandList->DispatchMesh(subset.Count, 1, 1);
                }
            }

			commandList->EndRenderPass();
		}

		commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

private:
    void PrepareRenderData(ICommandList* commandList, Scene::Scene& scene)
    {
        scene.OnUpdate(this->m_commonPasses);

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
        renderLight->SetDirection({ 0.0, -1.0f, 0.0f });
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

        frameData.SceneData.AtmosphereData.AmbientColour = float3(0.0f, 0.0f, 0.0f);

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
    RHI::MeshPipelineHandle m_pipeline;
    RHI::ShaderHandle m_meshShader;
    RHI::ShaderHandle m_pixelShader;
    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;
    PhxEngine::FirstPersonCameraController m_cameraController;
    RHI::BufferHandle m_frameConstantBuffer;

    RenderTargets m_renderTargets;
    std::unique_ptr<VisibilityBufferFillPass> m_visibilityBufferFillPass;
    std::unique_ptr<MeshletBufferFillPass> m_meshletBufferFillPass;
    std::shared_ptr<DeferredLightingPass> m_deferredLightingPass;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
    std::unique_ptr<Renderer::ToneMappingPass> m_toneMappingPass;

    RHI::TextureHandle m_depthTex;

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
    params.Name = "PhxEngine Example: Visibility Buffer";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        VisibilityRenderering example(root.get());
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
