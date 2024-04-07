#include "ViewportWidget.h"

#include "Renderer.h"
#include <imgui.h>
#include <PhxEngine/Core/Profiler.h>

using namespace PhxEngine;

void PhxEditor::ViewportWidget::OnImGuiRender()
{
    PHX_EVENT();

    auto* renderer = IRenderer::Ptr;
    // get viewport size
    uint32_t width = static_cast<uint32_t>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    uint32_t height = static_cast<uint32_t>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

    if (!this->m_viewportHandle.IsValid())
    {
        this->m_viewportHandle = renderer->ViewportCreate();
    }

    DirectX::XMFLOAT2 canvas = renderer->ViewportGetSize(this->m_viewportHandle);
    if (width != canvas.x || height != canvas.y)
    {
        renderer->ViewportResize(width, height, this->m_viewportHandle);
        canvas = renderer->ViewportGetSize(this->m_viewportHandle);
    }

    RHI::TextureHandle colourBuffer = renderer->ViewportGetColourBuffer(this->m_viewportHandle);
	ImGui::Image(static_cast<void*>(&colourBuffer), ImVec2{ canvas.x, canvas.y });
}
