#include "Renderer.h"

#include <PhxEngine/Core/Application.h>

using namespace PhxEngine;

PhxEditor::Renderer::Renderer()
{
    auto shaderPath = PhxEngine::Application::GetCurrentDir().parent_path().parent_path() / "PhxEditor" / "Assets" / "Shaders";
    std::shared_ptr<IFileSystem> baseSystem = FileSystemFactory::CreateNativeFileSystem();
    this->m_shaderFileSystem = FileSystemFactory::CreateRelativeFileSystem(baseSystem, shaderPath);
}

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

void PhxEditor::Renderer::LoadShaders()
{
    PHX_EVENT();

}

PhxEngine::Span<PhxEngine::RHI::ShaderHandle> PhxEditor::Renderer::GetShaderList() const
{
    return PhxEngine::Span<PhxEngine::RHI::ShaderHandle>();
}
