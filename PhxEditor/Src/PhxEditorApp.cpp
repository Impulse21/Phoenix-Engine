#include <PhxEngine/Core/EntryPoint.h>
#include <PhxEngine/Core/Application.h>

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/World/World.h>
#include <PhxEngine/World/Entity.h>
#include <PhxEngine/Resource/ResourceManager.h>
#include <PhxEngine/Resource/Mesh.h>

#include <imgui.h>
#include <imgui_internal.h>

using namespace PhxEngine;
class DemoLayer : public PhxEngine::Layer
{
public:
	DemoLayer()
		: Layer("DemoLayer")
	{}


	void OnImGuiRender() override 
	{
		PHX_EVENT();

#if true
		bool mainWindowBegun = this->BeginWindow();
#if false
		for (auto& widget : this->m_widgets)
		{
			widget->OnRender(mainWindowBegun);
		}
#endif
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
					// OpenProject();

					ImGui::Separator();

				if (ImGui::MenuItem("New World", "Ctrl+N"))
					// NewScene();

					if (ImGui::MenuItem("Save World", "Ctrl+S"))
						// SaveScene();

						if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
							// SaveSceneAs();

							ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					// Application::Get().Close();

					ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
					// ScriptEngine::ReloadAssembly();

					ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
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
#else
		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
					// OpenProject();

				ImGui::Separator();

				if (ImGui::MenuItem("New World", "Ctrl+N"))
					// NewScene();

				if (ImGui::MenuItem("Save World", "Ctrl+S"))
					// SaveScene();

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					// SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					// Application::Get().Close();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
					// ScriptEngine::ReloadAssembly();

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::Begin("Settings");
		ImGui::End();
		ImGui::ShowDemoWindow();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		this->m_viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		this->m_viewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		this->m_viewportFocused = ImGui::IsWindowFocused();
		this->m_viewportHovered = ImGui::IsWindowHovered();

		// Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		this->m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		// Get the Back buffer
		Application::GetInstance().GetGfxDevice().GetBackBuffer()
		uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				// OpenScene(path);
			}
			ImGui::EndDragDropTarget();
		}


		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();
#endif
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
					ImGui::DockBuilderDockWindow("Log", dock_down_id);
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
		bool m_viewportFocused = false;
		bool m_viewportHovered = false;
		DirectX::XMFLOAT2 m_viewportSize = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 m_viewportBounds[2];

};

class PhxEditor : public PhxEngine::Application
{
public:
	PhxEditor()
	{
		this->PushOverlay<DemoLayer>();
	}
};

PhxEngine::Application* PhxEngine::CreateApplication(ApplicationCommandLineArgs args)
{
	return new PhxEditor();
}

