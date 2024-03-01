#include <PhxEngine/EngineLoop.h>

#include <PhxEngine/Application.h>
#include <PhxEngine/Core/StopWatch.h>

using namespace PhxEngine;

// -- Engine Loop internals ---
namespace
{
	struct LoopConfig
	{
		int TargetFPS = 60;
	} m_loopConfig;

	bool m_isRunning = false;
	float m_targetFrameRateInv = 0;
	float m_deltaTimeAccumulator = 0.0f;

	StopWatch m_gameClock = {};

	void InitializeEngineServices()
	{

	}

	void Startup(IApplication* app)
	{
		m_targetFrameRateInv = 1.0f / static_cast<float>(m_loopConfig.TargetFPS);
		app->Startup();
	}

	void Shutdown(IApplication* app)
	{
		app->Shutdown();
	}

	void FixedUpdate()
	{

	}

	void Update(TimeStep const span)
	{

	}

	void Render()
	{

	}

	void Tick()
	{
		TimeStep deltaTimeSpan = m_gameClock.Elapsed();

		// Fixed update calculation
		{
			m_deltaTimeAccumulator += deltaTimeSpan.GetSeconds();
			if (m_deltaTimeAccumulator > 10)
			{
				// application probably lost control, fixed update would take too long
				m_deltaTimeAccumulator = 0;
			}

			while (m_deltaTimeAccumulator >= m_targetFrameRateInv)
			{
				FixedUpdate();
				m_deltaTimeAccumulator -= m_targetFrameRateInv;
			}
		}

		Update(deltaTimeSpan);
		Render();

		// -- Present Screen ---
	}
}

void PhxEngine::EngineLoop::Run(IApplication* app)
{
	InitializeEngineServices();
	Startup(app);

	m_gameClock.Begin();
	while (m_isRunning)
	{
		Tick();
	}

	app->Shutdown();
}