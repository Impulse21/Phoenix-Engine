#include <PhxEngine/Engine/EngineLoop.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/Renderer.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	void InitializeApplication(IEngineApp& app)
	{
		// Initialize SubSystems
		Core::Log::Initialize();
		app.Initialize();
	}

	void FinalizeApplication(IEngineApp& app)
	{
		app.Finalize();
	}
}

void EngineLoop::Run(IEngineApp& app)
{
	InitializeApplication(app);
	while (!app.IsShuttingDown())
	{
		app.OnTick();
	}
	FinalizeApplication(app);
}
