#include <PhxEngine/Engine/EngineCore.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <assert.h>

#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	// -- Globals ---
	std::unique_ptr<Core::IWindow> m_window;
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
		RHI::Initialize(RHI::GraphicsAPI::DX12);

		RHI::SwapChainDesc swapchainDesc = {
			.Width = m_window->GetWidth(),
			.Height = m_window->GetHeight(),
			.Fullscreen = false,
			.VSync = m_window->GetVSync(),
		};

		assert(false); // TODO Set up swapChain
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

		});

	}

	void EngineFinalize()
	{
		m_engineRunning.store(false);
		m_taskExecutor.wait_for_all();

		RHI::GetDynamic()->WaitForIdle();
		RHI::Finiailize();

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

#if false
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