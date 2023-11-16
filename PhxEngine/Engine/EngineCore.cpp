#include <Engine/EngineCore.h>

#include <Core/Log.h>
#include <Core/CommandLineArgs.h>
#include <Core/Memory.h>
#include <Renderer/Renderer.h>
#include <Core/EventDispatcher.h>
#include <assert.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	// -- Globals ---
	std::unique_ptr<Core::IWindow> m_window;
	RHI::SwapChain m_swapchain;
	tf::Executor m_taskExecutor;
	std::atomic_bool m_engineRunning = false;

	void EngineInitialize()
	{
		Core::Log::Initialize();
		PHX_LOG_CORE_INFO("Initailizing Engine Core");
		
		PhxEngine::Core::CommandLineArgs::Initialize();

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

		RHI::Initialize({});

		RHI::Factory::CreateSwapChain({
				.Width = m_window->GetWidth(),
				.Height = m_window->GetHeight(),
				.Fullscreen = false,
				.VSync = m_window->GetVSync(),
			}, 
			m_window->GetNativeWindowHandle(),
			m_swapchain);

		// -- Add on resize Event ---
		EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::Factory::CreateSwapChain({
					.Width = resizeEvent.GetWidth(),
					.Height = resizeEvent.GetHeight(),
					.Fullscreen = false,
					.VSync = m_window->GetVSync(),
				},
				m_window->GetNativeWindowHandle(),
				m_swapchain);
		});

	}

	void EngineFinalize()
	{
		m_engineRunning.store(false);
		m_taskExecutor.wait_for_all();

		RHI::WaitForIdle();
		RHI::Finalize();

		m_window.reset();

		Core::Log::Finialize();

		Core::ObjectTracker::Finalize();
		assert(0 == Core::SystemMemory::GetMemUsage());
		Core::SystemMemory::Cleanup();
	}
}

void PhxEngine::Run(IEngineApp& app)
{
	EngineInitialize();

	app.Initialize();

	while (!app.IsShuttingDown())
	{
		m_window->OnTick();

		// TODO: Add Delta Time
		app.OnUpdate();
		app.OnRender();

		// Compose final frame
		{
			// RHI::CommandList* composeCmdList = RHI::BeginCommandList();

			// RHI::SubmitCommands(composeCmdList);
		}

		RHI::Present({ m_swapchain });

#if 0
			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::Present, RHI::ResourceStates::RenderTarget)
				},
				composeCmdList);

			RHI::Color clearColour = {};
			gfxDevice->ClearTextureFloat(gfxDevice->GetBackBuffer(), clearColour, composeCmdList);

			app.OnCompose(composeCmdList);

			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::RenderTarget, RHI::ResourceStates::Present)
				},
				composeCmdList);
		}

		// Submit command lists and present
		gfxDevice->SubmitFrame();
#endif
	}

	app.Finalize();

	EngineFinalize();
}


Core::IWindow* PhxEngine::GetWindow()
{
	return m_window.get();
}

tf::Executor& PhxEngine::GetTaskExecutor()
{
	return m_taskExecutor;
}

PhxEngine::RHI::SwapChain& PhxEngine::GetSwapChain()
{
	return m_swapchain;
}