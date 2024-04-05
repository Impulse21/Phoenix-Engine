#include "ViewportWidget.h"

#include <imgui.h>

void PhxEditor::ViewportWidget::OnImGuiRender()
{
    // get viewport size
    uint32_t width = static_cast<uint32_t>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    uint32_t height = static_cast<uint32_t>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

	ImGui::Image(static_cast<void*>(&colourBuffer), ImVec2{ this->m_viewportSize.x, this->m_viewportSize.y });
}
