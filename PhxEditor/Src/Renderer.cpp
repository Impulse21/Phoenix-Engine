#include "Renderer.h"

#include <PhxEngine/Core/Application.h>
#include <PhxEngine/Core/VirtualFileSystem.h>


using namespace PhxEngine;

PhxEditor::Renderer::Renderer()
{
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

void PhxEditor::Renderer::LoadShadersAsync()
{
    Application::Executor().async("Loading Shaders", [this]() {
            this->LoadShaders();

        });
}

void PhxEditor::Renderer::LoadShaders()
{
    PHX_EVENT();

    std::filesystem::path shaderSourcePath = PhxEngine::Application::GetCurrentDir().parent_path().parent_path() / "PhxEditor" / "Assets" / "Shaders";
    RefCountPtr<FileAccess> triangleVS = FileAccess::Open(shaderSourcePath / "DrawTriangleVS.hlsl", AccessFlags::Read);
    RefCountPtr<FileAccess> trianglePS = FileAccess::Open(shaderSourcePath / "DrawTrianglePS.hlsl", AccessFlags::Read);

    // Check the Compile Directory to see if Shaders time stamp have changes

}

PhxEngine::Span<PhxEngine::RHI::ShaderHandle> PhxEditor::Renderer::GetShaderList() const
{
    return PhxEngine::Span<PhxEngine::RHI::ShaderHandle>();
}
