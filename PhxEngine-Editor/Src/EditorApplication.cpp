
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

// -- Add to engine ---
#include <PhxEngine/Core/WorkerThreadPool.h>
#include <PhxEngine/Core/Containers.h>
#include <PhxEngine/Renderer/ImGuiRenderer.h>
#include <imgui.h>

// -- Temp
#include <PhxEngine/RHI/PhxShaderCompiler.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// -- Engine Start-up ---
	{
		Core::Log::Initialize();
		Core::WorkerThreadPool::Initialize();
	}
	{ 
		// Initialize MemoryService?
		std::unique_ptr<IWindow> window = WindowFactory::CreateGlfwWindow({
			.Width = 2000,
			.Height = 1200,
			.VSync = false,
			.Fullscreen = false,
			});

		window->SetEventCallback([](Event& e) { EventDispatcher::DispatchEvent(e); });

		// Will initialize RHI on first time the window is created
		EventDispatcher::AddEventListener(EventType::WindowResize, [&](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::SwapChainDesc swapchainDesc = {
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.Fullscreen = false,
				.VSync = window->GetVSync(),
			};

			if (!RHI::GetGfxDevice())
			{
				RHI::Setup::Initialize({
					.Api = RHI::GraphicsAPI::DX12,
					.SwapChainDesc = swapchainDesc,
					.WindowHandle = window->GetNativeWindowHandle() });
			}
			else
			{
				RHI::GetGfxDevice()->ResizeSwapchain(swapchainDesc);
			}
			});

		window->Initialize();

		RHI::GfxDevice* gfxDevice = RHI::GetGfxDevice();
		ImGuiRenderer::Initialize(window.get(), gfxDevice);
		
		while (!window->ShouldClose())
		{
			window->OnTick();

			ImGuiRenderer::BeginFrame();
			RHI::CommandListHandle frameCmd = gfxDevice->BeginCommandList();

			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::Present, RHI::ResourceStates::RenderTarget)
				},
				frameCmd);

			RHI::Color clearColour = {};
			RHI::GetGfxDevice()->ClearTextureFloat(gfxDevice->GetBackBuffer(), clearColour, frameCmd);

			// Write Test to IMGUI Window
			static bool windowOpen = false;
			ImGui::Begin("Testing", &windowOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);

			ImGui::Text("Hello World");
			ImGui::End();
			ImGuiRenderer::Render(frameCmd);

			gfxDevice->TransitionBarriers(
				{
					RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::RenderTarget, RHI::ResourceStates::Present)
				},
				frameCmd);

			RHI::GetGfxDevice()->SubmitFrame();
		}

		ImGuiRenderer::Finalize();

	}
	// -- Finalize Block ---
	{
		RHI::GetGfxDevice()->WaitForIdle();
		RHI::GetGfxDevice()->Finalize();

		Core::WorkerThreadPool::Finalize();
		Core::Log::Finialize();

		Core::ObjectTracker::Finalize();
		assert(0 == SystemMemory::GetMemUsage());
		SystemMemory::Cleanup();
	}
}