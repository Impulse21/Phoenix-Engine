#include <PhxEngine/Engine/EngineCore.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/Renderer/Renderer.h>
using namespace PhxEngine;
using namespace PhxEngine::Core;

void EngineCore::Initialize()
{
	// Initialize SubSystems
	Core::Log::Initialize();
	Core::MemoryService::Initialize({});

	// Initialize RHI
	RHI::Initialize({
		.Api = RHI::GraphicsAPI::DX12,
		.EnableDebug = true,
		});

	Renderer::Initialize({
		.EnableImgui = true,
		});
}

void EngineCore::RunApplication(IEngineApp& app)
{
	app.Startup();
	while (!app.IsShuttingDown())
	{
		Renderer::BeginFrame();

		app.OnTick();

		// Renderer Tick? on differnet thread
		Renderer::Present();
	}

	app.Shutdown();

	// Finish any inflight Frames
	// RHI::GfxDevice::Impl->WaitForIdle();
	RHI::WaitForIdle();
}

void EngineCore::Finalize()
{
	Renderer::Finalize();
	RHI::Finalize();
}