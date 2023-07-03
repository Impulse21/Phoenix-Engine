#include "phxpch.h"
#include "PhxEngine/Core/Window.h"
#include "WindowGlfw.h"

using namespace PhxEngine::Core;

std::unique_ptr<IWindow> WindowFactory::CreateGlfwWindow(WindowSpecification const& spec)
{
	return std::make_unique<WindowGlfw>(spec);
}
