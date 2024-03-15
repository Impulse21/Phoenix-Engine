#include "RendererDefaultSubSystem.h"

#include <PhxEngine/Renderer/DisplaySubSystem.h>

using namespace PhxEngine;

void PhxEngine::RendererDefaultSubSystem::Startup()
{
	this->m_windowResizeHandler = [this](WindowResizeEvent const& e) {
			this->OnWindowResize(e);
		};

	EventBus::Subscribe<WindowResizeEvent>(this->m_windowResizeHandler);

	// Load Shaders
	// Draw Data
}

void PhxEngine::RendererDefaultSubSystem::Shutdown()
{
	EventBus::Unsubscribe<WindowResizeEvent>(this->m_windowResizeHandler);
}

void PhxEngine::RendererDefaultSubSystem::Update()
{
}

void PhxEngine::RendererDefaultSubSystem::Render()
{
}

void PhxEngine::RendererDefaultSubSystem::OnWindowResize(WindowResizeEvent const& e)
{
    // TODO:
}

DrawableHandle PhxEngine::RendererDefaultSubSystem::DrawableCreate(DrawableDesc const& desc)
{
    return DrawableHandle();
}

void PhxEngine::RendererDefaultSubSystem::DrawableUpdateTransform(DirectX::XMFLOAT4X4 transformMatrix)
{
}

void PhxEngine::RendererDefaultSubSystem::DrawableOverrideMaterial(MaterialHandle handle)
{
}

void PhxEngine::RendererDefaultSubSystem::DrawableDelete(DrawableHandle handle)
{
}

void PhxEngine::RendererDefaultSubSystem::MaterialCreate(MaterialDesc const& desc)
{
}

void PhxEngine::RendererDefaultSubSystem::MaterialUpdate(MaterialHandle handle, MaterialDesc const& desc)
{
}

void PhxEngine::RendererDefaultSubSystem::MaterialDelete(MaterialHandle handle)
{
}
