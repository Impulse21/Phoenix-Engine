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
#include <PhxEngine/Renderer/Forward3DRenderPath.h>
#include <PhxEngine/Core/FrameProfiler.h>
#include <PhxEngine/Engine/ImguiRenderer.h>

#include <imgui.h>
using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

class PhxEngineRuntimeApp : public ApplicationBase
{
private:

public:
    PhxEngineRuntimeApp(IPhxEngineRoot* root)
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
        this->m_forwardRenderer = std::make_unique<Renderer::Forward3DRenderPath>(
            this->GetGfxDevice(),
            this->m_commonPasses,
            this->m_shaderFactory,
            this->GetRoot()->GetFrameProfiler());

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza_Intel/Main/NewSponza_Main_glTF_002.gltf";
        this->m_loadAsync = true; // TODO: Command Queue issues when processing the Delete Queue.
        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_forwardRenderer->Initialize(this->GetRoot()->GetCanvasSize());

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

        Renderer::ResourceUpload indexUpload;
        Renderer::ResourceUpload vertexUpload;
        if (retVal)
        {
            this->m_scene.ConstructRenderData(commandList, indexUpload, vertexUpload);
        }

        Scene::Entity worldE = this->m_scene.CreateEntity("WorldComponent");
        auto& worldComponent = worldE.AddComponent<Scene::WorldEnvironmentComponent>();

        // Load IBL Textures
        std::filesystem::path texturePath = Core::Platform::GetExcecutableDir().parent_path() / "Assets/Textures/IBL/PaperMill_Ruins_E/PaperMill_IrradianceMap.dds";
        worldComponent.IblTextures[Scene::WorldEnvironmentComponent::IBLTextures::IrradanceMap] = m_textureCache->LoadTexture(texturePath, true, commandList);

        texturePath = Core::Platform::GetExcecutableDir().parent_path() / "Assets/Textures/IBL/PaperMill_Ruins_E/PaperMill_Skybox.dds";
        worldComponent.IblTextures[Scene::WorldEnvironmentComponent::IBLTextures::EnvMap] = m_textureCache->LoadTexture(texturePath, true, commandList);

        texturePath = Core::Platform::GetExcecutableDir().parent_path() / "Assets/Textures/IBL/PaperMill_Ruins_E/PaperMill_RadianceMap.dds";
        worldComponent.IblTextures[Scene::WorldEnvironmentComponent::IBLTextures::PreFilteredEnvMap] = m_textureCache->LoadTexture(texturePath, true, commandList);

        texturePath = Core::Platform::GetExcecutableDir().parent_path() / "Assets/Textures/IBL/BrdfLut.dds";
        auto brdfLut = m_textureCache->LoadTexture(texturePath, true, commandList);
        this->m_scene.SetBrdfLut(brdfLut);

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        indexUpload.Free();
        vertexUpload.Free();
        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->m_forwardRenderer->WindowResize({(float)e.GetWidth(), (float)e.GetHeight()});
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("PhxEngine Runtime", {});
        this->m_cameraController.OnUpdate(this->GetRoot()->GetWindow(), deltaTime, this->m_mainCamera);

        if (this->IsSceneLoaded())
        {
            this->m_scene.OnUpdate(this->m_commonPasses);
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

        this->m_forwardRenderer->Render(this->m_scene, this->m_mainCamera);
    }

    std::shared_ptr<Graphics::ShaderFactory> GetShaderFactory() { return this->m_shaderFactory; }

private:

private:
    std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
    std::shared_ptr<Renderer::Forward3DRenderPath> m_forwardRenderer;
    RHI::TextureHandle m_splashScreenTexture;

    Scene::Scene m_scene;
    Scene::CameraComponent m_mainCamera;
    PhxEngine::FirstPersonCameraController m_cameraController;

    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;

    float m_rotation = 0.0f;
};


class PhxEngineRuntimeUI : public PhxEngine::ImGuiRenderer
{
private:

public:
    PhxEngineRuntimeUI(IPhxEngineRoot* root, PhxEngineRuntimeApp* app)
        : ImGuiRenderer(root)
        , m_app(app)
    {
    }

    void BuildUI() override
    {
        if (m_app->IsSceneLoaded())
        {
            ImGui::Begin("Frame Stats");
            this->GetRoot()->GetFrameProfiler()->BuildUI();
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
    PhxEngineRuntimeApp* m_app;
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
        PhxEngineRuntimeApp app(root.get());
        if (app.Initialize())
        {
            root->AddPassToBack(&app);

            PhxEngineRuntimeUI userInterface(root.get(), &app);
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

    return 0;
}
