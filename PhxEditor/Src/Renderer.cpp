#include "Renderer.h"

#include <PhxEngine/Core/Application.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Resource/ResourceStore.h>
#include <PhxEngine/Resource/Shader.h>
#include "ShaderResourceHandler.h"

using namespace PhxEngine;

PhxEditor::Renderer::Renderer()
{
    ResourceStore::RegisterHandler<ShaderResourceHandler>();
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

    this->m_loadedShaders[ShaderTypes::VS_Triangle] = ResourceStore::GetResource<Shader>("/shaders/phx_engine/DrawTriangleVS.pso");
    this->m_loadedShaders[ShaderTypes::PS_Triangle] = ResourceStore::GetResource<Shader>("/shaders/phx_engine/DrawTrianglePS.pso");

    // Check the Compile Directory to see if Shaders time stamp have changes

}

PhxEngine::Span<PhxEngine::RHI::ShaderHandle> PhxEditor::Renderer::GetShaderList() const
{
    return PhxEngine::Span<PhxEngine::RHI::ShaderHandle>();
}
