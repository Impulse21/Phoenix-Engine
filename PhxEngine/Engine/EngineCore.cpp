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
	Renderer::AsyncGpuUploader m_asyncLoader;

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

		RHI::CreateSwapChain({
			.Width = m_window->GetWidth(),
			.Height = m_window->GetHeight(),
			.WindowHandle = m_window->GetNativeWindow(),
			.VSync = m_window->GetVSync(),
			.Fullscreen = false 
			},
			m_swapchain);

		// -- Add on resize Event ---
		EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::CreateSwapChain({
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.VSync = m_window->GetVSync(),
				.Fullscreen = false
				},
				m_swapchain);
		});

	}

	void EngineFinalize()
	{
		m_engineRunning.store(false);
		m_taskExecutor.wait_for_all();

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
		RHI::GfxDevice* gfxDevice = GetGfxDevice();
		{
			RHI::CommandListHandle composeCmdList = gfxDevice->BeginCommandList();

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
	}

	app.Finalize();

	EngineFinalize();
}


RHI::GfxDevice* PhxEngine::GetGfxDevice()
{
	return m_gfxDevice.get();
}

Core::IWindow* PhxEngine::GetWindow()
{
	return m_window.get();
}

tf::Executor& PhxEngine::GetTaskExecutor()
{
	return m_taskExecutor;
}

PhxEngine::Renderer::AsyncGpuUploader& PhxEngine::GetAsyncLoader()
{
	return m_asyncLoader;
}
