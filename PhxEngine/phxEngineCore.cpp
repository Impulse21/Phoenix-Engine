#include "pch.h"
#include "phxEngineCore.h"
#include "phxApplication.h"

#include <taskflow/taskflow.hpp>

using namespace phx;
using namespace phx::EngineCore;

namespace
{
	// -- Variables ---
	tf::Executor g_executor;
}

namespace
{
	void ThreadedTick(IApplication& app)
	{
		tf::Taskflow taskflow;
		auto [PreRenderTask, UpdateTask, RenderTask] = taskflow.emplace(  // create four tasks
			[&](tf::Subflow& subflow) { app.OnPreRender(&subflow); },
			[&](tf::Subflow& subflow) { app.OnUpdate(&subflow); },
			[&](tf::Subflow& subflow) { app.OnRender(&subflow); }
		);

		PreRenderTask.precede(UpdateTask, RenderTask);

		g_executor.run(taskflow).wait();
	}

	void SyncTick(IApplication& app)
	{
		app.OnPreRender();
		app.OnUpdate();
		app.OnRender();
	}
}

void phx::EngineCore::InitializeApplication(IApplication& app, PHX_UNUSED EngineParams const& desc)
{
	core::Log::Initialize();

	PHX_CORE_INFO("Engine Initializing");

	// Display number of Executor Tasks
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
