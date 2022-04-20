#include "GuiLayer.h"

#include <imgui.h>

#include <PhxEngine/Renderer/SceneComponents.h>

#include "EditorLayer.h"

void PhxEngine::Editor::GuiLayer::BuildUI()
{
	ImGui::Begin("Phx Engine");
	ImGui::Text("Welcome to the Phx Engine");

	if (ImGui::CollapsingHeader("Scene"))
	{
		ImGui::Checkbox("Disable IBL", &this->m_editorLayer->GetEditorSettings().DisableIbl);

		auto& scene = this->m_editorLayer->GetScene();

		if (ImGui::TreeNode("Light Components"))
		{
			for (int i = 0; i < scene.Lights.GetCount(); i++)
			{
				ECS::Entity e = scene.Lights.GetEntity(i);
				auto* nameComp = scene.Names.GetComponent(e);
				if (ImGui::TreeNode((void*)(intptr_t)i, "%s", nameComp ? nameComp->Name.c_str() : "Light Component"))
				{
					Renderer::LightComponent& lightComp = scene.Lights[i];
					ImGui::ColorEdit3("Colour", &lightComp.Colour.x);
					ImGui::DragFloat("Energy", &lightComp.Energy, 0.1f, 0.1f, 64.0f, "%.001f");
					ImGui::DragFloat("Range", &lightComp.Range, 0.1f, 1.0f, 1000.0f, "%.01f");
					static bool castShadows = lightComp.CastShadows();
					ImGui::Checkbox("Cast Shadows", &castShadows);

					lightComp.SetCastShadows(castShadows);

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}
	}
	ImGui::End();

	// ImGui::ShowDemoWindow();
}
