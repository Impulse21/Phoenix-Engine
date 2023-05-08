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
#include <imgui_internal.h>
using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

constexpr static uint32_t kNumLightInstances = 256;


#define DebugLights
// TODO: Move to a healper

namespace
{
template<typename T, typename UIFunc>
static void DrawComponent(std::string const& name, Scene::Entity entity, UIFunc uiFunc)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

    if (!entity.HasComponent<T>())
    {
        return;
    }

    auto& component = entity.GetComponent<T>();
    ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImGui::Separator();

    bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
    ImGui::PopStyleVar();

    ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
    if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
    {
        ImGui::OpenPopup("ComponentSettings");
    }

    bool removeComponent = false;
    if (ImGui::BeginPopup("ComponentSettings"))
    {
        if (ImGui::MenuItem("Remove component"))
        {
            removeComponent = true;
        }

        ImGui::EndPopup();
    }

    if (open)
    {
        uiFunc(component);
        ImGui::TreePop();
    }

    if (removeComponent)
    {
        entity.RemoveComponent<T>();
    }
}

static void DrawFloat3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    // float lineHeight = io.fonts->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    // ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X"))
        values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y"))
        values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z"))
        values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}

static void DrawFloat4Control(const std::string& label, DirectX::XMFLOAT4& values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    // float lineHeight = io.fonts->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    // ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X"))
        values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y"))
        values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z"))
        values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("W"))
        values.w = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}

}
struct AppSettings
{
    uint32_t NumPointLights = 128;
    uint32_t NumSpotLights = 128;
};

class ClusterLightingApp : public ApplicationBase
{
private:

public:
    ClusterLightingApp(IPhxEngineRoot* root)
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

        std::filesystem::path scenePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Models/Sponza/Sponza.gltf";

#ifdef ASYNC_LOADING
        this->m_loadAsync = true; // race condition when loading textures
#else
        this->m_loadAsync = false; // race condition when loading textures
#endif

        this->BeginLoadingScene(nativeFS, scenePath);

        this->m_deferredRenderer->Initialize(this->GetRoot()->GetCanvasSize());

        this->m_mainCamera.FoV = DirectX::XMConvertToRadians(60);

        Scene::TransformComponent t = {};
        t.LocalTranslation = { -4.0f, 2.0f, 0.3f };
        t.RotateRollPitchYaw({ 0.0f, DirectX::XMConvertToRadians(90), 0.0f});
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

        Scene::Entity matEntity = this->m_scene.CreateEntity("Light Mat");
        auto& mat = matEntity.AddComponent<Scene::MaterialComponent>();
        mat.BaseColour = { 0.0f, 0.0f, 0.0f, 1.0f };
        mat.Emissive = { 1.0f, 1.0f, 1.0f, 1.0f };

        this->m_debugLightOmniMesh = this->m_scene.CreateSphere(this->GetGfxDevice(), matEntity, 0.2f, 10u);
        this->m_debugLightSpotMesh = this->m_scene.CreateCube(this->GetGfxDevice(), matEntity, 0.2f);

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);

        this->m_scene.BuildRenderData(this->GetGfxDevice());

        return retVal;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->m_deferredRenderer->WindowResize({ (float)e.GetWidth(), (float)e.GetHeight() });
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("PhxEngine Exampe: Cluster Lighting", {});
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

    struct GeneratorSettings
    {
        int NumLights = 0;
        bool AddDebugMeshes = false;
    };

    void GenerateLights(GeneratorSettings const& settings)
    {
        auto& sceneBoundingBox = this->m_scene.GetBoundingBox();

        // Clear Existing
        this->m_scene.GetRegistry().clear<Scene::LightComponent>();
        assert(this->m_scene.GetAllEntitiesWith<Scene::LightComponent>().size() == 0);

        const auto& boundingBoxAABB = this->m_scene.GetBoundingBox();
        const auto& centre = boundingBoxAABB.GetCenter();

        // Spawn a ton of instances
        for (int i = 0; i < settings.NumLights; i++)
        {
            static size_t emptyNode = 0;
            auto entity = this->m_scene.CreateEntity("Light Node " + std::to_string(emptyNode++));

            auto& lightComp = entity.AddComponent<Scene::LightComponent>();
            lightComp.SetEnabled(true);
            lightComp.Type = (Scene::LightComponent::LightType)Core::Random::GetRandom(1, 2);
            lightComp.Colour = {
                Core::Random::GetRandom(0.0f, 1.0f),
                Core::Random::GetRandom(0.0f, 1.0f),
                Core::Random::GetRandom(0.0f, 1.0f),
                1.0f };

            lightComp.Intensity = Core::Random::GetRandom(0.0f, 100.0f);
            lightComp.Range = Core::Random::GetRandom(0.0f, 100.0f);
            lightComp.InnerConeAngle = Core::Random::GetRandom(0.0f, XM_PIDIV2 - 0.01f);
            lightComp.OuterConeAngle = Core::Random::GetRandom(lightComp.InnerConeAngle, XM_PIDIV2 - 0.01f);

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
            const float Bias = 2.0f;
            XMMATRIX translation = DirectX::XMMatrixTranslation(
                Core::Random::GetRandom(sceneBoundingBox.Min.x + Bias, sceneBoundingBox.Max.x - Bias),
                Core::Random::GetRandom(sceneBoundingBox.Min.y + Bias, sceneBoundingBox.Max.y - Bias),
                Core::Random::GetRandom(sceneBoundingBox.Min.z + Bias, sceneBoundingBox.Max.z - Bias));

            DirectX::XMStoreFloat4x4(&transform.WorldMatrix, scaleMatrix * DirectX::XMMatrixRotationAxis(axis, angle) * translation);
            transform.SetDirty();

            transform.ApplyTransform();
            transform.UpdateTransform();

            if (settings.AddDebugMeshes)
            {
                auto& debugMeshInst = entity.AddComponent<Scene::MeshInstanceComponent>();
                debugMeshInst.RenderBucketMask = Scene::MeshInstanceComponent::RenderType_Opaque;
                debugMeshInst.Color = { 0.0f, 0.0f, 0.0f, 1.0f };
                debugMeshInst.EmissiveColor = {
                    lightComp.Colour.x * lightComp.Intensity,
                    lightComp.Colour.y * lightComp.Intensity,
                    lightComp.Colour.z * lightComp.Intensity,
                    1.0f };

                debugMeshInst.Mesh = lightComp.Type == Scene::LightComponent::kOmniLight
                    ? this->m_debugLightOmniMesh
                    : this->m_debugLightSpotMesh;
            }
        }
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

    Scene::Entity m_debugLightOmniMesh;
    Scene::Entity m_debugLightSpotMesh;

    std::shared_ptr<Renderer::CommonPasses> m_commonPasses;
};


class ClusterLightingAppUI : public PhxEngine::ImGuiRenderer
{
private:

public:
    ClusterLightingAppUI(IPhxEngineRoot* root, ClusterLightingApp* app)
        : ImGuiRenderer(root)
        , m_app(app)
    {
    }

    void BuildUI() override
    {
        if (m_app->IsSceneLoaded())
        {
            ImGui::Begin("Options");
            if (ImGui::CollapsingHeader("Renderer"))
            {
                this->m_app->GetRenderer()->BuildUI();
            }

            if (ImGui::CollapsingHeader("Light Generation"))
            {
                static ClusterLightingApp::GeneratorSettings settings = {};
                ImGui::SliderInt("Num Lights", &settings.NumLights, 0, kNumLightInstances);
                ImGui::Checkbox("Add Debug Meshes", &settings.AddDebugMeshes);

                if (ImGui::Button("Generate"))
                {
                    this->m_app->GenerateLights(settings);
                }
            }

            ImGui::End();
        }
        else
        {
            ImGui::Begin("Loading Scene");
            ImGui::Text("The Scene Is currently Loading...");
            ImGui::End();
        }
    }

    void DrawEntityComponent(Scene::Entity entity)
    {
        if (entity.HasComponent<Scene::NameComponent>())
        {
            auto& name = entity.GetComponent<Scene::NameComponent>().Name;

            ImGui::Text(name.c_str());
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponent");
        }

        if (ImGui::BeginPopup("AddComponent"))
        {
            ImGui::EndPopup();
        }

        ImGui::PopItemWidth();


        DrawComponent<Scene::TransformComponent>("Transform", entity, [](auto& component) {
                DrawFloat3Control("Translation", component.LocalTranslation);
                DrawFloat4Control("Rotation", component.LocalRotation);
                DrawFloat3Control("Scale", component.LocalScale, 1.0f);
            });


        DrawComponent<Scene::LightComponent>("Light Component", entity, [](auto& component) {
           
                const char* items[] = { "Directional", "Omni", "Spot" };

                ImGui::Combo("Type", (int*)&component.Type, items, IM_ARRAYSIZE(items));
                bool isEnabled = component.IsEnabled();
                ImGui::Checkbox("Enabled", &isEnabled);
                component.SetEnabled(isEnabled);

                ImGui::ColorPicker3("Light Colour", &component.Colour.x, ImGuiColorEditFlags_NoSidePreview);
                

                ImGui::SliderFloat("Intensity", &component.Intensity, 0.0f, 1000.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
                ImGui::SliderFloat("Range", &component.Range, 0.0f, 1000.0f, "%.4f", ImGuiSliderFlags_Logarithmic);

                if (component.Type == Scene::LightComponent::kDirectionalLight || component.Type == Scene::LightComponent::kSpotLight)
                {
                    ImGui::InputFloat3("Direction", &component.Direction.x, "%.3f");
                }

                if (component.Type == Scene::LightComponent::kSpotLight)
                {
                    ImGui::SliderFloat("Inner Cone Angle", &component.InnerConeAngle, 0.0f, XM_PIDIV2 - 0.01f, "%e");
                    ImGui::SliderFloat("Outer Cone Angle", &component.OuterConeAngle, 0.0f, XM_PIDIV2 - 0.01f, "%e");
                }

                // Direction is starting from origin, so we need to negate it
                // Vec3 light(lightComponent.Direction.x, lightComponent.Direction.y, -lightComponent.Direction.z);
                // get/setLigth are helper funcs that you have ideally defined to manage your global/member objs
                // ImGui::Text("This is not working as expected. Do Not Use");
                // if (ImGui::gizmo3D("##Dir1", light /*, size,  mode */))
                {
                    //  lightComponent.Direction = { light.x, light.y, -light.z };
                }
            });
    }
private:
    ClusterLightingApp* m_app;
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
    params.Name = "PhxEngine";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        ClusterLightingApp app(root.get());
        if (app.Initialize())
        {
            root->AddPassToBack(&app);

            ClusterLightingAppUI userInterface(root.get(), &app);
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
