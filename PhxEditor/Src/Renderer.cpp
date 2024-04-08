#include "Renderer.h"

using namespace PhxEngine;

PhxEngine::ViewportHandle PhxEditor::Renderer::ViewportCreate()
{
    return PhxEngine::ViewportHandle();
}

void PhxEditor::Renderer::ViewportResize(uint32_t width, uint32_t height, PhxEngine::ViewportHandle viewport)
{
}

DirectX::XMFLOAT2 PhxEditor::Renderer::ViewportGetSize(PhxEngine::ViewportHandle viewport)
{
    return DirectX::XMFLOAT2();
}

PhxEngine::RHI::TextureHandle PhxEditor::Renderer::ViewportGetColourBuffer(PhxEngine::ViewportHandle handle)
{
    return PhxEngine::RHI::TextureHandle();
}

void PhxEditor::Renderer::OnUpdate()
{
    // Draw a triangle
}
