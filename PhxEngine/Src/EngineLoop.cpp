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
#include <PhxEngine/Resource/ResourceManager.h>
#include <PhxEngine/Resource/Mesh.h>

#include <thread>

using namespace PhxEngine;

// -- Engine Loop internals ---
namespace
{
	IntVar TargetFpsVar("Engine/Loop/Target Frame Rate", 60, 1);

	bool m_isRunning = true;
	float m_targetFrameRateInv = 0;
	float m_deltaTimeAccumulator = 0.0f;

	StopWatch m_gameClock = {};

	void StartupEngineServices()
	{
		Logger::Startup();
		EngineTuner::Startup();
		EngineMemory::Startup();
		ResourceManager::Startup();

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
		ResourceManager::Shutdown();
	}

	void PreRender(IApplication* app)
	{
		PHX_EVENT();
		app->PreRender();
	}

	void FixedUpdate(IApplication* app)
	{
		PHX_EVENT();
	}

	void Update(TimeStep const span, IApplication* app)
	{
		PHX_EVENT();
		EngineMemory::Update();
		RendererSubSystem::Ptr->Update();
	}

	void Render(IApplication* app)
	{
		PHX_EVENT();

		auto& gfxDevice = RHI::IGfxDevice::Ptr;
		RHI::CommandListHandle composeCmdList = gfxDevice->BeginCommandList();

		RHI::ScopedMarker("Clear Back Buffer", composeCmdList);
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
	}

	void Tick(IApplication* app)
	{
		DisplaySubSystem::Ptr->Update();
		EventBus::DispatchEvents();

		TimeStep deltaTimeSpan = m_gameClock.Elapsed();

		{
			PreRender(app);
		}

		// Update
		{
			{
				m_deltaTimeAccumulator += deltaTimeSpan.GetSeconds();
				if (m_deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					m_deltaTimeAccumulator = 0;
				}

				while (m_deltaTimeAccumulator >= m_targetFrameRateInv)
				{
					FixedUpdate(app);
					m_deltaTimeAccumulator -= m_targetFrameRateInv;
				}
			}

			Update(deltaTimeSpan, app);
		}

		// Render
		{
			Render(app);
			DisplaySubSystem::Ptr->Present();
		}
	}
}

void PhxEngine::EngineLoop::Run(IApplication* app)
{
	Startup(app);

	m_gameClock.Begin();
	while (m_isRunning)
	{
		PHX_FRAME("MainThread");
		Tick(app);
	}

	Shutdown(app);
}