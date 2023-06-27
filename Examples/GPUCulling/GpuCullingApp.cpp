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
#include <PhxEngine/Renderer/DeferredLightingPass.h>
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Renderer/GBuffer.h>
#include <PhxEngine/Engine/CameraControllers.h>
#include <PhxEngine/Renderer/RenderPath3DDeferred.h>
#include <PhxEngine/Renderer/RenderPath3DForward.h>
#include <PhxEngine/Core/Profiler.h>
#include <PhxEngine/Engine/ImguiRenderer.h>
#include <PhxEngine/Core/Memory.h>

#include <imgui.h>
using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

#if false
constexpr static uint32_t kNumInstances = 0;
#else
constexpr static uint32_t kNumInstances = 100000;
#endif

constexpr static DirectX::XMFLOAT2 kSceneExtent = { -200.0, 200.0 };

class GpuCullingApp : public ApplicationBase
{
private:

public:
    GpuCullingApp(IPhxEngineRoot* root)
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

        this->m_shaderFactory = std::make_shared<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath, "/Shaders");
        this->m_commonPasses = std::make_shared<Renderer::CommonPasses>(this->GetGfxDevice(), *this->m_shaderFactory);
        this->m_textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());
        this->m_deferredRenderer = std::make_unique<Renderer::RenderPath3DDeferred>(
            this->GetGfxDevice(),
            this->m_commonPasses,
            this->m_shaderFactory,
            this->GetRoot()->GetFrameProfiler());

        // std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza/Sponza.gltf";
        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/TestScenes/Monkey.gltf";

#ifdef ASYNC_LOADING
        this->m_loadAsync = true; // race condition when loading textures
#else
        this->m_loadAsync = false; // race condition when loading textures
#endif

        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_deferredRenderer->Initialize(this->GetRoot()->GetCanvasSize());

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);

        Scene::TransformComponent t = {};
        t.LocalTranslation = { 0.0f, 2.0f, -10.0f };
        t.SetDirty();
        t.UpdateTransform();

        this->m_mainCamera.TransformCamera(t);
        this->m_mainCamera.UpdateCamera();

        return true;
    }

    bool LoadScene(std::shared_ptr< Core::IFileSystem> fileSystem, std::filesystem::path sceneFilename) override
    {
        std::unique_ptr<Scene::ISceneLoader> sceneLoader = PhxEngine::Scene::CreateGltfSceneLoader();
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        this->m_scene.Initialize(&PhxEngine::Core::MemoryService::GetInstance().GetSystemAllocator());

        bool retVal = sceneLoader->LoadScene(
            fileSystem,
            this->m_textureCache,
            sceneFilename,
            commandList,
            this->m_scene);

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        auto meshView = this->m_scene.GetAllEntitiesWith<Scene::MeshComponent>();

        // Spawn a ton of instances
        for (int i = 0; i < kNumInstances; i++)
        {
            static size_t emptyNode = 0;
            auto entity = this->m_scene.CreateEntity("Scene Node " + std::to_string(emptyNode++));
            auto& instanceComponent = entity.AddComponent<Scene::MeshInstanceComponent>();
            instanceComponent.Mesh = meshView.front();
            instanceComponent.Color = {
                Core::Random::GetRandom(0.0f, 1.0f),
                Core::Random::GetRandom(0.0f, 1.0f),
                Core::Random::GetRandom(0.0f, 1.0f),
                1.0f };

            auto& transform = entity.GetComponent<Scene::TransformComponent>();

            float angle = Core::Random::GetRandom(0.0f, 1.0f) * DirectX::XM_2PI;
            XMVECTOR axis = XMVectorSet(
                Core::Random::GetRandom(-1.0f, 1.0f),
                Core::Random::GetRandom(-1.0f, 1.0f),
                Core::Random::GetRandom(-1.0f, 1.0f),
                0);

            float scale = Core::Random::GetRandom(1.0f, 4.0f);
            XMMATRIX scaleMatrix = XMMatrixScaling(
                scale,
                scale,
                scale);

            axis = DirectX::XMVector3Normalize(axis);
            XMMATRIX translation = DirectX::XMMatrixTranslation(
                Core::Random::GetRandom(kSceneExtent.x, kSceneExtent.y),
                Core::Random::GetRandom(kSceneExtent.x, kSceneExtent.y),
                Core::Random::GetRandom(kSceneExtent.x, kSceneExtent.y));

            DirectX::XMStoreFloat4x4(&transform.WorldMatrix, scaleMatrix * DirectX::XMMatrixRotationAxis(axis, angle) * translation);
            transform.SetDirty();

            transform.ApplyTransform();
            transform.UpdateTransform();
        }

        this->m_scene.BuildRenderData(this->GetGfxDevice());

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->m_deferredRenderer->WindowResize({ (float)e.GetWidth(), (float)e.GetHeight() });
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("PhxEngine Exampe: GPU Culling", {});
        this->m_cameraController.OnUpdate(this->GetRoot()->GetWindow(), deltaTime, this->m_mainCamera);

        if (this->IsSceneLoaded())
        {
            this->m_scene.OnUpdate(this->m_commonPasses, this->m_deferredRenderer->GetSettings().GISettings.EnableDDGI);
        }
    }

    void RenderSplashScreen() override
    {
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();
        commandList->BeginRenderPassBackBuffer();
        // TODO: Render Splash Screen or some text about loading.
        commandList->EndRenderPass();
        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

    void RenderScene() override
    {
        this->m_mainCamera.Width = this->GetRoot()->GetCanvasSize().x;
        this->m_mainCamera.Height = this->GetRoot()->GetCanvasSize().y;
        this->m_mainCamera.UpdateCamera();

        this->m_deferredRenderer->Render(this->m_scene, this->m_mainCamera);
    }

    std::shared_ptr<Graphics::ShaderFactory> GetShaderFactory() { return this->m_shaderFactory; }
    std::shared_ptr<Renderer::RenderPath3DDeferred> GetRenderer() { return this->m_deferredRenderer; }
private:

private:
    std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
    std::shared_ptr<Renderer::RenderPath3DDeferred> m_deferredRenderer;
    RHI::TextureHandle m_splashScreenTexture;

    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;
    PhxEngine::FirstPersonCameraController m_cameraController;

    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;

    float m_rotation = 0.0f;
};


class GpuCullingAppUI : public PhxEngine::ImGuiRenderer
{
private:

public:
    GpuCullingAppUI(IPhxEngineRoot* root, GpuCullingApp* app)
        : ImGuiRenderer(root)
        , m_app(app)
    {
    }

    void BuildUI() override
    {
        if (m_app->IsSceneLoaded())
        {
            ImGui::Begin("Renderer");
            ImGui::Text("Currently rendering %d monkeys", kNumInstances + 1);
            ImGui::Separator();
            this->m_app->GetRenderer()->BuildUI();
            ImGui::End();
        }
        else
        {
            ImGui::Begin("Loading Scene");
            ImGui::Text("The Scene Is currently Loading...");
            ImGui::End();
        }
    }

private:
    GpuCullingApp* m_app;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    PhxEngine::Core::MemoryService::GetInstance().Initialize({
            .MaximumDynamicSize = PhxGB(1)
        });

    EngineParam params = {};
    params.Name = "PhxEngine Example: GPU Culling";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        GpuCullingApp app(root.get());
        if (app.Initialize())
        {
            root->AddPassToBack(&app);

            GpuCullingAppUI userInterface(root.get(), &app);
            if (userInterface.Initialize(*app.GetShaderFactory()))
            {
                root->AddPassToBack(&userInterface);
            }

            root->Run();
            root->RemovePass(&app);
            root->RemovePass(&userInterface);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    PhxEngine::Core::MemoryService::GetInstance().Finalize();
    return 0;
}
