#pragma once

#include <atomic>
#include "StopWatch.h"

#include "RHI/PhxRHI.h"

namespace PhxEngine::Core
{
	struct CommandLineArgs
	{

	};

	class Application
	{
	public:
		virtual void Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice);
		virtual void Finalize();

		void RunFrame();

		//void FixedUpdate();
		void Update(TimeStep deltaTime);
		void Render();
		void Compose();

	private:
		bool m_isInitialized;
		std::atomic_bool m_initializationComplete;

		StopWatch m_stopWatch;

		PhxEngine::RHI::IGraphicsDevice* m_graphicsDevice = nullptr;
	};

	// Defined by client
	Application* CreateApplication(CommandLineArgs args);
}

