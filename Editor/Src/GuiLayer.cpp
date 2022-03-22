#include "GuiLayer.h"

#include <imgui.h>

void PhxEngine::Editor::GuiLayer::BuildUI()
{
	ImGui::Begin("Phx Engine");
	ImGui::Text("Welcome to the Phx Engine");
	ImGui::End();
}
