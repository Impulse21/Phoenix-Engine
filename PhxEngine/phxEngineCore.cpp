#include "pch.h"
#include "phxEngineCore.h"
#include "phxApplication.h"

using namespace phx;
using namespace phx::EngineCore;

namespace
{
	void ThreadedTick(UNUSED IApplication& app)
	{

	}

	void SyncTick(IApplication& app)
	{
		app.OnPreRender();
		app.OnUpdate();
		app.OnRender();
	}
}

namespace
{
	// -- Variables ---
}

void phx::EngineCore::InitializeApplication(IApplication& app, UNUSED EngineParams const& desc)
{
	app.OnStartup();
}

void phx::EngineCore::Tick(IApplication& app)
{
	if (app.EnableMultiThreading())
		ThreadedTick(app);
	else
		SyncTick(app);
}

void phx::EngineCore::FinializeApplcation(IApplication& app)
{
	app.OnShutdown();
}
