#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Renderer/ImGuiRenderer.h>
#include <PhxEngine/Engine/World.h>
#include "Widgets.h"
#include "GltfModelLoader.h"

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
	class EditorApplication : public IEngineApp
	{
	public:
        EditorApplication()
        {

        }

		void Initialize() override
		{
			PhxEngine::GetTaskExecutor().silent_async([this]() {
				Renderer::ImGuiRenderer::Initialize(GetWindow(), GetGfxDevice(), true);
			    Renderer::ImGuiRenderer::EnableDarkThemeColours();
			    std::unique_ptr<IFileSystem> fileSystem = CreateNativeFileSystem();

			    auto postMsgWithLog = [this](const char* msg) {
			    	PHX_LOG_INFO(msg);
			    	this->m_loadingScreen.SetCaption(msg);
			    };

			    postMsgWithLog("Initializing Widgets");
			    this->m_widgets.emplace_back(std::make_shared<Editor::ConsoleLogWidget>());
			    this->m_widgets.emplace_back(std::make_shared<Editor::MenuBar>());
			    this->m_widgets.emplace_back(std::make_shared<Editor::ProfilerWidget>());

			    std::string worldFilename;
			    if (CommandLineArgs::GetString("world", worldFilename))
			    {
			    	// Load World
			    	postMsgWithLog("Loading Scene");
			    	PHX_LOG_INFO("Loading Scene '%s'", worldFilename.c_str());
			    	if (std::filesystem::path(worldFilename).extension() == ".gltf")
			    	{
			    		Core::StopWatch stopWatch;

			    		Editor::GltfModelLoader loader;

			    		std::future<bool> loadTask = loader.LoadModelAsync(worldFilename, fileSystem.get(), this->m_activeWorld);

                        loadTask.wait();

			    		Core::TimeStep loadTime = stopWatch.Elapsed();
			    		PHX_LOG_INFO("Loading scene took %dms", loadTime.GetMilliseconds());
			    	}
			    }
			    else
			    {
			    	postMsgWithLog("Loading Default World");

                    postMsgWithLog("Checking if meshes are on disk, if not loaded");

			    }

			    this->m_isInitialize.store(true);
			});
		}

		void Finalize() override
		{
			Renderer::ImGuiRenderer::Finalize();
		}

		bool IsShuttingDown() override
		{
			return GetWindow()->ShouldClose();
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

            bool mainWindowBegun = this->BeginWindow();
            for (auto& widget : this->m_widgets)
            {
                widget->OnRender(mainWindowBegun);
            }

            // TODO: Render Viewport
            ImGui::Begin("World", &mainWindowBegun);
            ImGui::Text("World");
            ImGui::End();

            ImGui::Begin("Properties", &mainWindowBegun);
            ImGui::Text("Properties");
            ImGui::End();
            ImGui::Begin("Viewport", &mainWindowBegun);
            ImGui::Text("Viewport");
            ImGui::End();

            if (mainWindowBegun)
                ImGui::End();
		}

		void OnCompose(RHI::CommandListHandle composeCmdList) override
		{
			Renderer::ImGuiRenderer::Render(composeCmdList);
		}

        template<typename _TWidget>
        std::shared_ptr<_TWidget> GetWidget()
        {
            for (const auto& widget : this->m_widgets)
            {
                if (_TWidget* retVal = std::dynamic_pointer_cast<_TWidget>(widget))
                {
                    return retVal;
                }
            }

            return nullptr;

        }

	private:
        bool BeginWindow()
        {
            const auto window_flags =
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus;

            // Set window position and size
            // float offset_y = widget_menu_bar ? (widget_menu_bar->GetHeight() + widget_menu_bar->GetPadding()) : 0;
            float offset_y = 0;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + offset_y));
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - offset_y));
            ImGui::SetNextWindowViewport(viewport->ID);

            // Set window style
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.0f);

            // Begin window
            std::string name = "##main_window";
            bool open = true;
            bool windowBegun = ImGui::Begin(name.c_str(), &open, window_flags);
            ImGui::PopStyleVar(3);

            // Begin dock space
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable && windowBegun)
            {
                // Dock space
                const auto window_id = ImGui::GetID(name.c_str());
                if (!ImGui::DockBuilderGetNode(window_id))
                {
                    // Reset current docking state
                    ImGui::DockBuilderRemoveNode(window_id);
                    ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
                    ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

                    // DockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
                    ImGuiID dock_main_id = window_id;
                    ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
                    ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.6f, nullptr, &dock_right_id);
                    ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
                    ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id, ImGuiDir_Right, 0.6f, nullptr, &dock_down_id);

                    // Dock windows
                    ImGui::DockBuilderDockWindow("World", dock_right_id);
                    ImGui::DockBuilderDockWindow("Properties", dock_right_down_id);
                    ImGui::DockBuilderDockWindow(Editor::ConsoleLogWidget::WidgetName.data(), dock_down_id);
                    ImGui::DockBuilderDockWindow("Assets", dock_down_right_id);
                    ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

                    ImGui::DockBuilderFinish(dock_main_id);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
                ImGui::PopStyleVar();
            }

            return windowBegun;
        }

    private:
        LoadingScreen m_loadingScreen;
        std::vector<std::shared_ptr<Editor::IWidgets>> m_widgets;
        std::atomic_bool m_isInitialize = false;
        PhxEngine::World::World m_activeWorld;
	};
}

PHX_CREATE_APPLICATION(EditorApplication);
