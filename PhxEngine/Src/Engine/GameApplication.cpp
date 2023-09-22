#include <PhxEngine/Engine/GameApplication.h>
#include <PhxEngine/Core/Window.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/RHI/PhxShaderCompiler.h>


using namespace PhxEngine;
using namespace PhxEngine::Core;

void GameApplication::Initialize()
{
	// Initialize MemoryService?
	this->m_window = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});
	this->m_window->SetEventCallback([this](Event& e) { EventDispatcher::DispatchEvent(e); });

	// Will initialize RHI on first time the window is created
	EventDispatcher::AddEventListener(EventType::WindowResize, [this](Event const& e) {

			const WindowResizeEvent& resizeEvent = static_cast<const WindowResizeEvent&>(e);
			RHI::SwapChainDesc swapchainDesc = {
				.Width = resizeEvent.GetWidth(),
				.Height = resizeEvent.GetHeight(),
				.Fullscreen = false,
				.VSync = this->m_window->GetVSync(),
			};

			if (!RHI::GetGfxDevice())
			{
				RHI::Setup::Initialize({
					.Api = RHI::GraphicsAPI::DX12,
					.SwapChainDesc = swapchainDesc,
					.WindowHandle = this->m_window->GetNativeWindowHandle() });
			}
			else
			{
				RHI::GetGfxDevice()->ResizeSwapchain(swapchainDesc);
			}
		});

	this->m_window->Initialize();

	this->Startup();
}

void PhxEngine::GameApplication::Finalize()
{
	RHI::Setup::Finalize();
}

bool PhxEngine::GameApplication::IsShuttingDown()
{
	return this->m_window->ShouldClose();
}

void PhxEngine::GameApplication::OnTick()
{
	// Have we initialized yet?
	this->m_window->OnTick();

	auto* gfxDevice = RHI::GetGfxDevice();

	RHI::CommandListHandle frameCmd = gfxDevice->BeginCommandList();
	gfxDevice->TransitionBarriers(
		{ 
			RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::Present, RHI::ResourceStates::RenderTarget)
		},
		frameCmd);

	this->OnRender();

	RHI::Color clearColour = {};
	RHI::GetGfxDevice()->ClearTextureFloat(gfxDevice->GetBackBuffer(), clearColour, frameCmd);

	gfxDevice->TransitionBarriers(
		{
			RHI::GpuBarrier::CreateTexture(gfxDevice->GetBackBuffer(), RHI::ResourceStates::RenderTarget, RHI::ResourceStates::Present)
		},
		frameCmd);
	gfxDevice->SubmitFrame();
}

void PhxEngine::GameApplication::Startup()
{
	std::shared_ptr<IFileSystem> fileSystem = CreateNativeFileSystem();
	CreateRelativeFileSystem(fileSystem, "");
	RHI::ShaderCompiler::Compile("Testing", {});

	this->m_imguiRenderer = std::make_unique<ImGuiRenderer>();
	this->m_imguiRenderer->Initialize();
}
void PhxEngine::GameApplication::Shutdown()
{
}
