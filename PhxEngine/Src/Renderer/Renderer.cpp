#include <PhxEngine/Renderer/Renderer.h>

using namespace PhxEngine;
using namespace PhxEngine::Renderer;

namespace
{
	bool m_enableImgui;
};

void PhxEngine::Renderer::Initialize(RendererParameters const& parameters)
{
	// Register Events we want to listen too.

	// On Window resize Event

	m_enableImgui = parameters.EnableImgui;

}

void PhxEngine::Renderer::Finalize()
{
}

void PhxEngine::Renderer::BeginFrame()
{
}

void PhxEngine::Renderer::Present()
{
}
