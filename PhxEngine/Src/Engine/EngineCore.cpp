#include <PhxEngine/Engine/EngineCore.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <assert.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	// -- Globals ---
	std::unique_ptr<Core::IWindow> m_window;
	std::unique_ptr<RHI::GfxDevice> m_gfxDevice;
	enki::TaskScheduler m_taskScheduler;

	// -- IO Tasks --
	struct RunPinnedTaskLoopTask : enki::IPinnedTask 
	{
		void Execute() override 
		{
			auto& taskScheduler = PhxEngine::GetTaskScheduler();
			while (taskScheduler.GetIsRunning() && execute)
			{
				taskScheduler.WaitForNewPinnedTasks(); // this thread will 'sleep' until there are new pinned tasks
				taskScheduler.RunPinnedTasks();
			}
		}

		bool execute = true;
	} m_runPinnedTaskLoopTask;

	//
	//
	struct AsynchronousLoadTask : enki::IPinnedTask 
	{
		void Execute() override 
		{
			// Do file IO
			while (execute) 
			{
				// TODO: async_loader->update(nullptr);
			}
		}

		// AsynchronousLoader* async_loader;
		bool execute = true;
	} m_asynchronousLoadTask;

	void EngineInitialize()
	{
		PHX_LOG_CORE_INFO("Initailizing Engine Core");
		Core::Log::Initialize();

		enki::TaskSchedulerConfig config;
		// for pinned async loader
		config.numTaskThreadsToCreate += 1;
		m_taskScheduler.Initialize(config);

		m_runPinnedTaskLoopTask.threadNum = m_taskScheduler.GetNumTaskThreads() - 1;
		m_taskScheduler.AddPinnedTask(&m_runPinnedTaskLoopTask);

		m_asynchronousLoadTask.threadNum = m_runPinnedTaskLoopTask.threadNum;
		m_taskScheduler.AddPinnedTask(&m_asynchronousLoadTask);

		m_window = WindowFactory::CreateGlfwWindow({
			.Width = 2000,
			.Height = 1200,
			.VSync = false,
			.Fullscreen = false,
			});
		m_window->SetEventCallback([](Event& e) { EventDispatcher::DispatchEvent(e); });
		m_window->Initialize();

		m_gfxDevice = RHI::Factory::CreateD3D12Device();
		RHI::SwapChainDesc swapchainDesc = {
			.Width = m_window->GetWidth(),
			.Height = m_window->GetHeight(),
			.Fullscreen = false,
			.VSync = m_window->GetVSync(),
		};

		m_gfxDevice->Initialize(swapchainDesc, m_window->GetNativeWindowHandle());

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

	void EngineFinalize()
	{
		m_asynchronousLoadTask.execute = false;
		m_runPinnedTaskLoopTask.execute = false;
		m_taskScheduler.WaitforAllAndShutdown();

		m_gfxDevice->WaitForIdle();
		m_gfxDevice->Finalize();

		m_gfxDevice.reset();
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

	// Engine Initialization
	enki::TaskSet initializeAppAsyncTask([&app](enki::TaskSetPartition range_, uint32_t threadnum_) {
		app.InitializeAsync();
	});

	PhxEngine::GetTaskScheduler().AddTaskSetToPipe(&initializeAppAsyncTask);

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

enki::TaskScheduler& PhxEngine::GetTaskScheduler()
{
	return m_taskScheduler;
}
