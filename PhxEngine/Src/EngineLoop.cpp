#include <PhxEngine/EngineLoop.h>

#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/Profiler.h>
#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/Core/EventBus.h>

#include <PhxEngine/Application.h>
#include <PhxEngine/EngineTuner.h>
#include <PhxEngine/EngineMemory.h>

#include <Renderer/RendererDefaultSubSystem.h>
#include <Renderer/GlfWDisplaySubSystem.h>
#include <PhxEngine/RHI/PhxRHI.h>

#include <thread>
#include <taskflow/taskflow.hpp>

using namespace PhxEngine;

// -- Engine Loop internals ---
namespace
{
	IntVar TargetFpsVar("Engine/Loop/Target Frame Rate", 60, 1);

	bool m_isRunning = true;
	float m_targetFrameRateInv = 0;
	float m_deltaTimeAccumulator = 0.0f;

	StopWatch m_gameClock = {};
	tf::Executor m_executor;

	void StartupEngineServices()
	{
		Logger::Startup();
		EngineTuner::Startup();
		EngineMemory::Startup();

		// TODO: Introduce dependecy injectiong for things like the Graphics Device...
		DisplaySubSystem::Ptr = new GlfWDisplaySubSystem();
		DisplaySubSystem::Ptr->Startup();

		RendererSubSystem::Ptr = new RendererDefaultSubSystem();
		RendererSubSystem::Ptr->Startup();

	}

	void Startup(IApplication* app)
	{
		StartupEngineServices();

		m_targetFrameRateInv = 1.0f / static_cast<float>(TargetFpsVar);

		app->Startup();

		EventBus::Subscribe<WindowCloseEvent>([](WindowCloseEvent const& e) { m_isRunning = false; });
	}

	void Shutdown(IApplication* app)
	{
		app->Shutdown();

		RendererSubSystem::Ptr->Shutdown();
		DisplaySubSystem::Ptr->Shutdown();
	}

	void FixedUpdate(IApplication* app, tf::Subflow& subflow)
	{
		PHX_EVENT();
	}

	void Update(IApplication* app, TimeStep const span, tf::Subflow& subflow)
	{
		PHX_EVENT();
		EngineMemory::Update();
		RendererSubSystem::Ptr->Update();
	}

	void Render(IApplication* app, tf::Subflow& subflow)
	{
		PHX_EVENT();

		auto& gfxDevice = RHI::IGfxDevice::Ptr;
		RHI::CommandListHandle composeCmdList = gfxDevice->BeginCommandList();

		gfxDevice->TransitionBarriers(
			{
				RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::Present, RHI::ResourceStates::RenderTarget)
			},
			composeCmdList);

		RHI::Color clearColour = {};
		gfxDevice->ClearTextureFloat(gfxDevice->GetBackBuffer(), clearColour, composeCmdList);

		gfxDevice->TransitionBarriers(
			{
				RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::RenderTarget, RHI::ResourceStates::Present)
			},
			composeCmdList);

		// -- Present Screen ---
		DisplaySubSystem::Ptr->Present();
	}
	void Tick(IApplication* app)
	{
		tf::Taskflow frameTaskflow;
		TimeStep deltaTimeSpan = m_gameClock.Elapsed();

		DisplaySubSystem::Ptr->Update();
		EventBus::DispatchEvents();
		deltaTimeSpan = m_gameClock.Elapsed();

		tf::Task preRenderTask = frameTaskflow.emplace(
			[&](tf::Subflow& subflow) {
			}).name("Pre-Render");
		

		
		tf::Task updateTask = frameTaskflow.emplace(
			[&](tf::Subflow& subflow) {
				m_deltaTimeAccumulator += deltaTimeSpan.GetSeconds();
				if (m_deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					m_deltaTimeAccumulator = 0;
				}

				while (m_deltaTimeAccumulator >= m_targetFrameRateInv)
				{
					FixedUpdate(app, subflow);
					m_deltaTimeAccumulator -= m_targetFrameRateInv;
				}

				Update(app, deltaTimeSpan, subflow);
			}).name("Update");
				
		tf::Task renderTask = frameTaskflow.emplace(
			[&](tf::Subflow& subflow) {
					Render(app, subflow);
				}).name("Render");


		// Build Frame dependencies
		preRenderTask.precede(updateTask, renderTask);

		m_executor.run(frameTaskflow).wait();
	}
}

void PhxEngine::EngineLoop::Run(IApplication* app)
{
	Startup(app);

	m_gameClock.Begin();
	while (m_isRunning)
	{
		PHX_FRAME("MainThread");
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		Tick(app);
	}

	Shutdown(app);
}