#include <PhxEngine/Engine/EngineCore.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/WorkerThreadPool.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <assert.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;

namespace
{
	std::unique_ptr<Core::IWindow> m_window;
	std::unique_ptr<RHI::GfxDevice> m_gfxDevice;

	void EnginePreInitialize()
	{
		Core::Log::Initialize();
		Core::WorkerThreadPool::Initialize();

	}

	void EnginePostFinalize()
	{
		Core::WorkerThreadPool::Finalize();
		Core::Log::Finialize();

		Core::ObjectTracker::Finalize();
		assert(0 == Core::SystemMemory::GetMemUsage());
		Core::SystemMemory::Cleanup();
	}
}

void PhxEngine::Initialize()
{
	PHX_LOG_CORE_INFO("Initailizing Engine Core");
	m_window = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});

	m_window->SetEventCallback([](Event& e) { EventDispatcher::DispatchEvent(e); });

	EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

		const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
	// TODO: Data Drive this
	RHI::SwapChainDesc swapchainDesc = {
		.Width = resizeEvent.GetWidth(),
		.Height = resizeEvent.GetHeight(),
		.Fullscreen = false,
		.VSync = m_window->GetVSync(),
	};

	if (!m_gfxDevice)
	{
		m_gfxDevice = RHI::Factory::CreateD3D12Device();
		m_gfxDevice->Initialize(swapchainDesc, m_window->GetNativeWindowHandle());
	}
	else
	{
		m_gfxDevice->ResizeSwapchain(swapchainDesc);
	}
		});

	m_window->Initialize();
}

void PhxEngine::Run(IEngineApp& app)
{
	EnginePreInitialize();

	app.Initialize();
	while (!app.IsShuttingDown())
	{
		m_window->OnTick();

		// TODO: Add Delta Time
		app.OnUpdate();
		app.OnRender();

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
	}

	app.Finalize();

	EnginePostFinalize();
}

void PhxEngine::Finalize()
{
	m_gfxDevice->WaitForIdle();
	m_gfxDevice->Finalize();

	m_gfxDevice.reset();
	m_window.reset();
}

RHI::GfxDevice* PhxEngine::GetGfxDevice()
{
	return m_gfxDevice.get();
}

Core::IWindow* PhxEngine::GetWindow()
{
	return m_window.get();
}
