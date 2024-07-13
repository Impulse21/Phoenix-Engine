#include "pch.h"
#include "phxEngineCore.h"
#include "phxApplication.h"
#include "RHI/phxRHI.h"

#include <taskflow/taskflow.hpp>

using namespace phx;

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

void phx::Engine::Initialize(IApplication& app, PHX_UNUSED EngineParams const& desc)
{
	core::Log::Initialize();

	PHX_CORE_INFO("Engine Initializing");

	rhi::Config rhiConfig = {};
	rhi::InitializeWindows(rhiConfig);

	// Display number of Executor Tasks
	app.OnStartup();
}

void phx::Engine::Tick(IApplication& app)
{
	if (app.EnableMultiThreading())
		ThreadedTick(app);
	else
		SyncTick(app);
}

void phx::Engine::Finialize(IApplication& app)
{
	app.OnShutdown();

	rhi::Finalize();
}
