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
#include <PhxEngine/Renderer/GBufferFillPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Renderer/GBuffer.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

struct RenderTargets : public GBufferRenderTargets
{
    // Extra
    RHI::TextureHandle FinalColourBuffer;

    void Initialize(
        RHI::IGraphicsDevice* gfxDevice,
        DirectX::XMFLOAT2 const& size)
    {
        GBufferRenderTargets::Initialize(gfxDevice, size);

        RHI::TextureDesc desc = {};
        desc.Width = std::max(size.x, 1.0f);
        desc.Height = std::max(size.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
        desc.InitialState = RHI::ResourceStates::ShaderResource;
        desc.IsBindless = true;
        desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
        desc.DebugName = "Final Colour Buffer";
        desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::ShaderResource | BindingFlags::UnorderedAccess;

        this->FinalColourBuffer = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);
    }

    void Free(RHI::IGraphicsDevice* gfxDevice) override
    {
        GBufferRenderTargets::Free(gfxDevice);
        gfxDevice->DeleteTexture(this->FinalColourBuffer);
    }
};


// TODO: MOVE TO ENGINE AND UPDATE DEFERREED RENDERER
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

        void FillGBuffer(RenderTargets const& renderTargets)
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

class Shadows : public ApplicationBase
{
private:

public:
    Shadows(IPhxEngineRoot* root)
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

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza/Sponza.gltf";
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());

        this->m_gbufferFillPass = std::make_unique<GBufferFillPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_gbufferFillPass->Initialize(*this->m_shaderFactory);

        this->m_deferredLightingPass = std::make_shared<DeferredLightingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_deferredLightingPass->Initialize(*this->m_shaderFactory);

        this->m_frameConstantBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer({
            .Usage = RHI::Usage::Default,
            .Binding = RHI::BindingFlags::ConstantBuffer,
            .InitialState = RHI::ResourceStates::ConstantBuffer,
            .SizeInBytes = sizeof(Shader::Frame),
            .DebugName = "Frame Constant Buffer",
            });

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);

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

            indexUpload.Free();
            vertexUpload.Free();
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
        this->m_renderTargets.Free(this->GetGfxDevice());
        this->m_renderTargets.Initialize(this->GetGfxDevice(), this->GetRoot()->GetCanvasSize());
        this->m_gbufferFillPass->WindowResized();
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->m_rotation += deltaTime.GetSeconds() * 1.1f;
        this->GetRoot()->SetInformativeWindowTitle("Example: Deferred Rendering", {});
    }

    void RenderScene() override
    {
        this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
        this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
        this->m_mainCamera.UpdateCamera();

        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        {
            auto _ = commandList->BeginScopedMarker("Preare Frame Data");
            this->PrepareRenderData(commandList, this->m_scene);
        }

        // The camera data doesn't match whats in the CPU and what's in the GPU.
        Shader::Camera cameraData = {};
        cameraData.ViewProjection = this->m_mainCamera.ViewProjection;
        cameraData.ViewProjectionInv = this->m_mainCamera.ViewProjectionInv;
        cameraData.ProjInv = this->m_mainCamera.ProjectionInv;
        cameraData.ViewInv = this->m_mainCamera.ViewInv;

        {
            auto _ = commandList->BeginScopedMarker("GBuffer Fill");

            this->m_gbufferFillPass->BeginPass(commandList, this->m_frameConstantBuffer, cameraData, this->m_renderTargets);
            static thread_local DrawQueue drawQueue;
            drawQueue.Reset();

            // Look through Meshes and instances?
            auto view = this->m_scene.GetAllEntitiesWith<Scene::MeshInstanceComponent, Scene::AABBComponent>();
            for (auto e : view)
            {
                auto [instanceComponent, aabbComp] = view.get<Scene::MeshInstanceComponent, Scene::AABBComponent>(e);

                auto& meshComponent = this->m_scene.GetRegistry().get<Scene::MeshComponent>(instanceComponent.Mesh);
                if (meshComponent.RenderBucketMask & Scene::MeshComponent::RenderType_Transparent)
                {
                    continue;
                }

                const float distance = Core::Math::Distance(this->m_mainCamera.Eye, aabbComp.BoundingData.GetCenter());

                drawQueue.Push((uint32_t)instanceComponent.Mesh, (uint32_t)e, distance);
            }

            drawQueue.SortOpaque();

            RenderViews(commandList, this->m_gbufferFillPass.get(), this->m_scene, drawQueue, true);
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
        renderLight->SetDirection({ 0.1, -0.2, 1.0f });
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
    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;

    RHI::BufferHandle m_frameConstantBuffer;

    RenderTargets m_renderTargets;
    std::unique_ptr<GBufferFillPass> m_gbufferFillPass;
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
    params.Name = "PhxEngine Example: Shadows";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        Shadows example(root.get());
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
