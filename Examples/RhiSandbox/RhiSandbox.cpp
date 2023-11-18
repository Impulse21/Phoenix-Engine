#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/EventDispatcher.h>

using namespace PhxEngine;

namespace
{
	std::unique_ptr<Core::IWindow> window;
	void Initialize()
	{
		Core::Log::Initialize();
		RHI::Initialize(RHI::GraphicsAPI::DX12);

		window = Core::WindowFactory::CreateGlfwWindow({
		   .Width = 2000,
		   .Height = 1200,
		   .VSync = false,
		   .Fullscreen = false,
			});

		window->SetEventCallback([](Core::Event& e) { Core::EventDispatcher::DispatchEvent(e); });
		window->Initialize();

		RHI::DynamicRHI* rhi = RHI::GetDynamic();
		Core::EventDispatcher::AddEventListener(Core::EventType::WindowResize, [&](Core::Event const& e) 
			{
				const Core::WindowResizeEvent& resizeEvent = static_cast<const Core::WindowResizeEvent&>(e);
				RHI::SwapChainDesc swapchainDesc = {
					.Width = resizeEvent.GetWidth(),
					.Height = resizeEvent.GetHeight(),
					.Fullscreen = false,
					.VSync = window->GetVSync() };
				rhi->ResizeSwapchain(swapchainDesc);
			});
	}

	void Finalize()
	{
		RHI::Finiailize();
		Core::Log::Finialize();
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Initialize();

	RHI::DynamicRHI* rhi = RHI::GetDynamic();


	Finalize();
	// Create SwapChain
}