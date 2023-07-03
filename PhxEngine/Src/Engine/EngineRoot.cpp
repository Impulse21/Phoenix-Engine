#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Engine/EngineRoot.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/PhxRenderer.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

constexpr static bool sPixCapture = false;

bool PhxEngine::EngineInitialize(EngineParam const& params)
{
	Core::Log::Initialize();

	PHX_LOG_CORE_INFO("Initializing Engine");

	PhxEngine::Core::MemoryService::GetInstance().Initialize({
			.MaximumDynamicSize = params.MaximumDynamicSize
		});

	RHI::Initialize({});

	// Initialize Subsystems
	Renderer::Initialize();

	return true;
}

bool PhxEngine::EngineRun(IEngineApplication& app)
{
	PHX_LOG_CORE_INFO("Engine Run");
	app.Initialize();
	while (!app.ShuttingDown())
	{
		app.RunFrame();
	}

	app.Finalize();

	RHI::GfxDevice::Impl->WaitForIdle();
	return true;
}

bool PhxEngine::EngineFinalize()
{
	PHX_LOG_CORE_INFO("Finalizing Engine Shutdown");

	Renderer::Finialize();

	PhxEngine::RHI::Finalize();
	PhxEngine::Core::MemoryService::GetInstance().Finalize();

	Core::Log::Finalize();
	return true;
}