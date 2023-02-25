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
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Engine/CameraControllers.h>
#include <PhxEngine/Renderer/GeometryPasses.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

struct Settings
{
    bool EnableGpuCulling = false;
    bool EnableMeshlets = false;
};

struct GpuCullPass
{
public:
    GpuCullPass(RHI::IGraphicsDevice* gfxDevice)
        : m_gfxDevice(gfxDevice)
    {}

    void Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_computeShader = factory.CreateShader(
            "IndirectCull/CullPass.hlsl",
            {
                .Stage = RHI::ShaderStage::Compute,
                .DebugName = "CullPassCS",
            });
        assert(this->m_computeShader.IsValid());
    }

    void Dispatch()
    {
        if (!this->m_computeShader.IsValid())
        {
            this->m_pipeline = this->m_gfxDevice->CreateComputePipeline(
                {
                    .ComputeShader = this->m_computeShader,
                });
        }

        // Dispatch
    }

private:
    RHI::ShaderHandle m_computeShader;
    RHI::ComputePipelineHandle m_pipeline;
    RHI::IGraphicsDevice* m_gfxDevice;
};


class IndirectDraw : public ApplicationBase
{
public:
    IndirectDraw(IPhxEngineRoot* root)
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
        rootFilePath->Mount("/Shaders/IndirectDraw", appShadersRoot);

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza/Sponza.gltf";
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_gpuCullPass = std::make_unique<GpuCullPass>(this->GetGfxDevice());
        this->m_gpuCullPass->Initialize(*this->m_shaderFactory);

        this->m_toneMappingPass = std::make_unique<Renderer::ToneMappingPass>(this->GetGfxDevice(), this->m_commonPasses);
        this->m_toneMappingPass->Initialize(*this->m_shaderFactory);

        // TODO LOAD IBL for nicer shading
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
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("Example: GPU Culling", {});
        this->m_cameraController.OnUpdate(this->GetRoot()->GetWindow(), deltaTime, this->m_mainCamera);
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

        if (this->m_settings.EnableGpuCulling)
        {
            auto _ = commandList->BeginScopedMarker("GPU Cull");
            this->m_gpuCullPass->Dispatch();
        }

        {
            auto _ = commandList->BeginScopedMarker("GBuffer Fill");

            if (this->m_settings.EnableGpuCulling)
            {
                if (!this->m_commandSignatureHandle.IsValid())
                {
#if false
                    this->m_commandSignatureHandle = this->m_gfxDevice->CreateCommandSignature<VisibilityIndirectDraw>({
                   .ArgDesc =
                       {
                           {.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
                           {.Type = IndirectArgumentType::DrawIndex }
                       },
                   .PipelineType = PipelineType::Gfx,
                   .GfxHandle = this->m_gbufferFillPass->GetPipeline();
                        });
#endif
                }
            }
            else
            {
                static thread_local Renderer::DrawQueue drawQueue;
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

            }
        }

#if false
        this->m_toneMappingPass->Render(
            commandList,
            this->m_renderTargets.ColourBuffer,
            commandList->GetRenderPassBackBuffer(),
            this->GetRoot()->GetCanvasSize());
#endif
        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

    Settings& GetSettings() { return this->m_settings; }

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
    Settings m_settings = {};
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::GraphicsPipelineHandle m_pipeline;
    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;
    PhxEngine::FirstPersonCameraController m_cameraController;
    RHI::BufferHandle m_frameConstantBuffer;

    std::unique_ptr<GpuCullPass> m_gpuCullPass;
    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
    std::unique_ptr<Renderer::ToneMappingPass> m_toneMappingPass;


    RHI::CommandSignatureHandle m_commandSignatureHandle;

    struct GBufferFillIndirectDraw
    {
        uint32_t GeometryIndex;
        RHI::IndirectDrawArgInstanced DrawAgs = {};
    };
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
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        IndirectDraw example(root.get());
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