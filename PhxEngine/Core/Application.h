#pragma once

#include <atomic>
#include "StopWatch.h"

#include "Core/Canvas.h"
#include "Core/Platform.h"
#include "Graphics/RHI/PhxRHI.h"

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
		void Compose(PhxEngine::RHI::CommandListHandle cmdList);

		void SetWindow(Core::Platform::WindowHandle windowHandle, bool isFullscreen = false);

	private:
		bool m_isInitialized;
		std::atomic_bool m_initializationComplete;

		StopWatch m_stopWatch;

		PhxEngine::RHI::IGraphicsDevice* m_graphicsDevice = nullptr;
		Core::Platform::WindowHandle m_windowHandle = nullptr;
		Core::Canvas m_canvas;

		// RHI Resources
		PhxEngine::RHI::CommandListHandle m_composeCommandList;
	};

	// Defined by client
	Application* CreateApplication(CommandLineArgs args);
}

