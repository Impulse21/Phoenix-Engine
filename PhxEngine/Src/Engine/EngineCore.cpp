#include <PhxEngine/Engine/EngineCore.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <assert.h>

#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	// -- Globals ---
	std::unique_ptr<Core::IWindow> m_window;
	RHI::SwapChainRef m_swapChain;
	tf::Executor m_taskExecutor;
	LinearAllocator m_scratchAllocator;
	std::atomic_bool m_engineRunning = false;

	void EngineInitialize()
	{
		Core::Log::Initialize();
		PHX_LOG_CORE_INFO("Initailizing Engine Core");
		
		PhxEngine::Core::CommandLineArgs::Initialize();

		// -- Initialize Task Scheduler ---
		PHX_LOG_CORE_INFO("Engine has %d workers ", m_taskExecutor.num_workers());

		// -- Create Window ---
		m_window = WindowFactory::CreateGlfwWindow({
			.Width = 2000,
			.Height = 1200,
			.VSync = false,
			.Fullscreen = false,
			});
		m_window->SetEventCallback([](Event& e) { EventDispatcher::DispatchEvent(e); });
		m_window->Initialize();

		// -- Create GFX Device ---
		RHI::Initialize(RHI::GraphicsAPI::DX12);

		RHI::SwapChainDesc swapchainDesc = {
			.Width = m_window->GetWidth(),
			.Height = m_window->GetHeight(),
			.Fullscreen = false,
			.VSync = m_window->GetVSync(),
		};

		m_swapChain = RHI::GetDynamic()->CreateSwapChain(swapchainDesc, m_window->GetNativeWindowHandle());

		// -- Add on resize Event ---
		EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			// TODO: Data Drive this
			RHI::SwapChainDesc swapchainDesc = {
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.Fullscreen = false,
				.VSync = m_window->GetVSync(),
			};

			RHI::GetDynamic()->ResizeSwapChain(m_swapChain, swapchainDesc);
		});

		m_scratchAllocator.Initialize(PhxMB(1));
	}

	void EngineFinalize()
	{
		m_engineRunning.store(false);
		m_taskExecutor.wait_for_all();

		RHI::GetDynamic()->WaitForIdle();
		m_swapChain.Reset();
		RHI::Finiailize();

		m_window.reset();
		m_scratchAllocator.Finalize();

		Core::Log::Finialize();

		Core::ObjectTracker::Finalize();
		assert(0 == Core::SystemMemory::GetMemUsage());
		Core::SystemMemory::Cleanup();
	}
}

void PhxEngine::Run(IEngineApp& app)
{
	EngineInitialize();

	app.Initialize();

	RHI::CommandListRef gfxCmdList = RHI::GetDynamic()->CreateCommandList();
	while (!app.IsShuttingDown())
	{
		m_scratchAllocator.Clear();
		m_window->OnTick();

		// TODO: Add Delta Time
		app.OnUpdate();

		gfxCmdList->Reset();
		Renderer::RgBuilder rgBuilder(&m_scratchAllocator);
		app.OnRender(rgBuilder, gfxCmdList.Get());

		RHI::GetDynamic()->ExecuteCommandLists({ gfxCmdList });
		RHI::GetDynamic()->Present(m_swapChain.Get());

#if false
		// Compose final frame
		RHI::GfxDevice* gfxDevice = GetGfxDevice();
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
#endif
	}

	app.Finalize();

	EngineFinalize();
}

Core::IWindow* PhxEngine::GetWindow()
{
	return m_window.get();
}

RHI::ISwapChain* PhxEngine::GetSwapChain()
{
	return m_swapChain.Get();
}

tf::Executor& PhxEngine::GetTaskExecutor()
{
	return m_taskExecutor;
}