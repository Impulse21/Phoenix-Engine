#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <PhxEngine/Resource/ResourceLoader.h>

#include "Widgets.h"
#include "ViewportWidget.h"
#include "Renderer.h"

using namespace PhxEngine;

void PhxEditor::EditorLayer::OnAttach()
{
	PHX_EVENT();
	PhxEngine::IRenderer::Ptr = phx_new(Renderer);
	PhxEngine::IRenderer::Ptr->LoadShaders();

	this->RegisterWidget<ConsoleLogWidget>();
}

void PhxEditor::EditorLayer::OnDetach()
{
	PHX_EVENT();
	phx_delete PhxEngine::IRenderer::Ptr;
	PhxEngine::IRenderer::Ptr = nullptr;
}

void PhxEditor::EditorLayer::OnUpdate(PhxEngine::TimeStep ts)
{
	PhxEngine::IRenderer::Ptr->OnUpdate();
}

void PhxEditor::EditorLayer::OnImGuiRender()
{
	bool mainWindowBegun = this->BeginWindow();

	auto dir = Application::GetCurrentWorkingDirectory();

	for (auto& widget : this->m_widgets)
	{
		widget->OnImGuiRender(mainWindowBegun);
	}

	// TODO: Render Viewport
	ImGui::Begin("World", &mainWindowBegun);
	ImGui::Text("World");
	ImGui::End();

	ImGui::Begin("Properties", &mainWindowBegun);
	ImGui::Text("Properties");
	ImGui::End();

	if (mainWindowBegun)
		ImGui::End();
}

bool PhxEditor::EditorLayer::BeginWindow()
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
			ImGui::DockBuilderDockWindow(WidgetTitle<ConsoleLogWidget>::Name(), dock_down_id);
			ImGui::DockBuilderDockWindow("Assets", dock_down_right_id);
			ImGui::DockBuilderDockWindow(WidgetTitle<ViewportWidget>::Name(), dock_main_id);

			ImGui::DockBuilderFinish(dock_main_id);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::PopStyleVar();
	}

	return windowBegun;
}
