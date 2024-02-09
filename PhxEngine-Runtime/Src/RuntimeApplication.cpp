#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Renderer/ImGuiRenderer.h>
#include <PhxEngine/Engine/World.h>
#include <PhxEngine/Assets/PhxPktFileManager.h>


#include <imgui.h>
#include <imgui_internal.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
    class LoadingScreen
    {
    public:
        void SetCaption(std::string_view view)
        {
            std::scoped_lock _(this->m_lock);
            this->m_caption = view;
        }

        void OnRender()
        {
            ImGuiIO& io = ImGui::GetIO();
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent backgroundconst float PAD = 10.0f;

            static bool showWindow = true;
            ImGui::Begin(
                "Intializing",
                &showWindow,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBackground);
            ImGui::Text("Loading...");
            ImGui::Text(this->m_caption.c_str());
            ImGui::End();
        }

    private:
        std::string m_caption;
        std::mutex m_lock;
    };
    class RuntimeApplication : public IEngineApp
    {
    public:
        RuntimeApplication()
        {

        }

        void Initialize() override
        {
            Engine::GetTaskExecutor().silent_async([this]() {
                Renderer::ImGuiRenderer::Initialize(Engine::GetWindow(), Engine::GetGfxDevice(), true);

                Renderer::ImGuiRenderer::EnableDarkThemeColours();
                std::unique_ptr<IFileSystem> fileSystem = CreateNativeFileSystem();

                auto postMsgWithLog = [this](const char* msg) {
                    PHX_LOG_INFO(msg);
                    this->m_loadingScreen.SetCaption(msg);
                };

                // Register Path
                Assets::PhxPktFileManager::AddFile("C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Baked\\NewSponza_Main_glTF.phxpkt");
                this->m_isInitialize.store(true);
            });
        }

        void Finalize() override
        {
            Renderer::ImGuiRenderer::Finalize();
        }

        bool IsShuttingDown() override
        {
            return Engine::GetWindow()->ShouldClose();
        }

        void OnUpdate() override
        {
            if (!this->m_isInitialize.load())
            {
                return;
            }
        }

        void OnRender() override
        {
            Renderer::ImGuiRenderer::BeginFrame();
            if (!this->m_isInitialize.load())
            {
                m_loadingScreen.OnRender();
                return;
            }

            Assets::PhxPktFileManager::RenderDebugWindow();

            // Render Registries
        }

        void OnCompose(RHI::CommandListHandle composeCmdList) override
        {
            Renderer::ImGuiRenderer::Render(composeCmdList);
        }

    private:
        LoadingScreen m_loadingScreen;
        std::atomic_bool m_isInitialize = false;
        PhxEngine::World::World m_activeWorld;
    };
}

PHX_CREATE_APPLICATION(RuntimeApplication);
