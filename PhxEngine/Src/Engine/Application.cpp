#include <PhxEngine/Engine/Application.h>

#include <PhxEngine/Engine/EngineRoot.h>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <assert.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{

	void EngineInitialize()
	{

	}

	void EngineFinalize()
	{
	}
}

void PhxEngine::Run(IEngineApp& app)
{
	struct GaurdedShutdown
	{
		~GaurdedShutdown()
		{
			Engine::Finalize();
		}
	} _gaurdedShutdown;

	app.PreInitialize();

	Engine::Initialize();

	app.Initialize();

	while (!app.IsShuttingDown())
	{
		Engine::Tick();

		app.OnUpdate();
		app.OnRender();

		// Compose final frame
		RHI::GfxDevice* gfxDevice = Engine::GetGfxDevice();
		{
			RHI::CommandListHandle composeCmdList = gfxDevice->BeginCommandList();

			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::Present, RHI::ResourceStates::RenderTarget)
				},
				composeCmdList);

			RHI::Color clearColour = {};
			gfxDevice->ClearTextureFloat(gfxDevice->GetBackBuffer(), clearColour, composeCmdList);

			app.OnCompose(composeCmdList);

			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::RenderTarget, RHI::ResourceStates::Present)
				},
				composeCmdList);
		}

		// Submit command lists and present
		gfxDevice->SubmitFrame();
	}

	app.Finalize();

	// Engine shutdown is invoked by the gaurded shutdown.
}