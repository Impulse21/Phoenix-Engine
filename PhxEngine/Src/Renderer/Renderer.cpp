#include <PhxEngine/Renderer/Renderer.h>

#include <PhxEngine/RHI/RHI.h>

using namespace PhxEngine;
using namespace PhxEngine::Renderer;

namespace
{
	RHI::SwapChain m_swapchain;
	bool m_enableImgui;
};

void PhxEngine::Renderer::Initialize(RendererParameters const& parameters)
{
	// Register Events we want to listen too.

	// On Window resize Event

	m_enableImgui = parameters.EnableImgui;

}
