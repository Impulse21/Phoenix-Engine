
#include "PhxEngine/Core/Window.h"
#include "WindowGlfw.h"
#include <PhxEngine/Core/Memory.h>

using namespace PhxEngine::Core;

IWindow* WindowFactory::CreateGlfwWindow(WindowSpecification const& spec)
{
	return phx_new(WindowGlfw)(spec);
}
