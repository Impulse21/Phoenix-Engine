#include <PhxEngine/Engine/GameApplication.h>
#include <PhxEngine/Core/Window.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

void GameApplication::Startup()
{
	// Initialize MemoryService?
	this->m_window = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});
	this->m_window->SetEventCallback([this](Event& e) { EventDispatcher::DispatchEvent(e); });

	// Will initialize RHI on first time the window is created
	EventDispatcher::AddEventListener(EventType::WindowResize, [this](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::SwapChainDesc swapchainDesc = {
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.Fullscreen = false,
				.VSync = this->m_window->GetVSync(),
			};

			if (!RHI::GetGfxDevice())
			{
				RHI::Setup::Initialize({
					.Api = RHI::GraphicsAPI::DX12,
					.SwapChainDesc = swapchainDesc,
					.WindowHandle = this->m_window->GetNativeWindowHandle() });
			}
			else
			{
				RHI::GetGfxDevice()->ResizeSwapchain(swapchainDesc);
			}
		});

	this->m_window->Initialize();
}

void PhxEngine::GameApplication::Shutdown()
{
	RHI::Setup::Finalize();
}

bool PhxEngine::GameApplication::IsShuttingDown()
{
	return this->m_window->ShouldClose();
}

void PhxEngine::GameApplication::OnTick()
{
	this->m_window->OnTick();

}
