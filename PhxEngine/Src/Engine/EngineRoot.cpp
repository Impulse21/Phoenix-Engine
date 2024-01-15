#include <PhxEngine/Engine/EngineRoot.h>

#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Threading.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <PhxEngine/Core/Window.h>

using namespace PhxEngine::Core;

void PhxEngine::Engine::Initialize()
{
	Core::Log::Initialize();
	PHX_LOG_CORE_INFO("Initailizing Engine Core");

	PhxEngine::Core::CommandLineArgs::Initialize();

	PhxEngine::Core::Threading::SetMainThread();

	// -- Initialize Task Scheduler ---
	PHX_LOG_CORE_INFO("Engine has %d workers ", m_taskExecutor.num_workers());

	// -- Create Window ---
	m_window = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});
	m_window->SetEventCallback([](Event& e) { EventDispatcher::DispatchEvent(e); });
	m_window->Initialize();

	// -- Create GFX Device ---
	m_gfxDevice = RHI::GfxDeviceFactory::Create(RHI::GraphicsAPI::DX12);
	RHI::SwapChainDesc swapchainDesc = {
		.Width = m_window->GetWidth(),
		.Height = m_window->GetHeight(),
		.Fullscreen = false,
		.VSync = m_window->GetVSync(),
	};

	m_gfxDevice->Initialize(swapchainDesc, m_window->GetNativeWindowHandle());

	// -- Add on resize Event ---
	EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

		const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
		// TODO: Data Drive this
		RHI::SwapChainDesc swapchainDesc = {
			.Width = resizeEvent.GetWidth(),
			.Height = resizeEvent.GetHeight(),
			.Fullscreen = false,
			.VSync = m_window->GetVSync(),
		};

		m_gfxDevice->ResizeSwapchain(swapchainDesc);
		});
}

void PhxEngine::Engine::Finalize()
{
	m_engineRunning.store(false);
	m_taskExecutor.wait_for_all();

	m_gfxDevice->WaitForIdle();
	m_gfxDevice->Finalize();


	phx_delete(m_gfxDevice);
	m_gfxDevice = nullptr;

	phx_delete(m_window);

	Core::Log::Finialize();

	Core::ObjectTracker::Finalize();
	assert(0 == Core::SystemMemory::GetMemUsage());
	Core::SystemMemory::Cleanup();
}

void PhxEngine::Engine::Tick()
{
	Engine::m_window->OnTick();
}
