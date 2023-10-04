
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Renderer/ImGuiRenderer.h>
#include <imgui.h>
#include <imgui_internal.h>


#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	class EditorApplication : public IEngineApp
	{
	public:
		void Initialize() override
		{
			Renderer::ImGuiRenderer::Initialize(GetWindow(), GetGfxDevice(), true);
            Renderer::ImGuiRenderer::EnableDarkThemeColours();
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
		}

		void OnRender() override
		{
			Renderer::ImGuiRenderer::BeginFrame();
            this->BeginWindow();

            if (this->m_editorBegin)
                ImGui::End();
		}

		void OnCompose(RHI::CommandListHandle composeCmdList) override
		{
			Renderer::ImGuiRenderer::Render(composeCmdList);
		}

	private:
        void BeginWindow()
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
            this->m_editorBegin = ImGui::Begin(name.c_str(), &open, window_flags);
            ImGui::PopStyleVar(3);

            // Begin dock space
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable && this->m_editorBegin)
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
                    ImGui::DockBuilderDockWindow("Console", dock_down_id);
                    ImGui::DockBuilderDockWindow("Assets", dock_down_right_id);
                    ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

                    ImGui::DockBuilderFinish(dock_main_id);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
                ImGui::PopStyleVar();
            }
        }

        private:
            bool m_editorBegin= false;
	};
}

PHX_CREATE_APPLICATION(EditorApplication);
