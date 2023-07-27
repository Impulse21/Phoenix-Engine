
#include "PhxEngine/Core/Window.h"
#include "WindowGlfw.h"

using namespace PhxEngine::Core;

std::unique_ptr<IWindow> WindowFactory::CreateGlfwWindow(IPhxEngineRoot* engRoot, WindowSpecification const& spec)
{
	return std::make_unique<WindowGlfw>(engRoot, spec);
}
