#include <PhxEngine/Engine/GameApplication.h>
#include <PhxEngine/Core/Window.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

void GameApplication::Startup()
{
	// Initialize MemoryService?
	
	// Single RHI Interface, or use some sort of global Objects
	RHI::Initialize({});

	this->m_window = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});
	this->m_window->SetEventCallback([this](Event& e) { EventDispatcher::DispatchEvent(e); });

	EventDispatcher::AddEventListener(EventType::WindowResize, [this](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::SwapChainDesc swapchainDesc = {
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.Fullscreen = false,
				.VSync = this->m_window->GetVSync(),
			};

			RHI::Factory::CreateSwapChain(swapchainDesc, this->m_window->GetNativeWindow(), this->m_swapchain);
		});

	this->m_window->Initialize();
}

void PhxEngine::GameApplication::Shutdown()
{
	RHI::WaitForIdle();

	RHI::Finalize();
}

bool PhxEngine::GameApplication::IsShuttingDown()
{
	return this->m_window->ShouldClose();;
}

void PhxEngine::GameApplication::OnTick()
{
	RHI::Present(this->m_swapchain);
}
