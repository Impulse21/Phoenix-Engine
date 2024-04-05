#include <PhxEngine/Renderer/Window.h>
#include "GlfWWindow.h"

std::unique_ptr<PhxEngine::IWindow> PhxEngine::WindowFactory::Create()
{
	return std::make_unique<PhxEngine::GlfWWindow>();
}