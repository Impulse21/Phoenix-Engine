#include "Renderer.h"

PhxEngine::ViewportHandle PhxEditor::Renderer::ViewportCreate()
{
    return PhxEngine::ViewportHandle();
}

void PhxEditor::Renderer::ViewportResize(uint32_t width, uint32_t height, PhxEngine::ViewportHandle viewport)
{
}

DirectX::XMFLOAT2 PhxEngine::IRenderer::ViewportGetSize(ViewportHandle viewport)
{
    return DirectX::XMFLOAT2();
}

PhxEngine::RHI::TextureHandle PhxEditor::Renderer::ViewportGetColourBuffer(PhxEngine::ViewportHandle handle)
{
    return PhxEngine::RHI::TextureHandle();
}
