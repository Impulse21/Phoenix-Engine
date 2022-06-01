#include "phxpch.h"

#include "Application.h"

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

void Application::Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice)
{
	this->m_graphicsDevice = graphicsDevice;

	Graphics::ShaderFactory factory(this->m_graphicsDevice, "shaders/", "../PhxEngine/Shaders/");
	this->m_shaderStore.PreloadShaders(factory);

	this->m_composeCommandList = this->m_graphicsDevice->CreateCommandList();

	this->m_isInitialized = true;
}

void Application::Finalize()
{
	this->m_imguiRenderer.Finalize();
}

void Application::Tick()
{
	// TODO: Initialization?
	TimeStep deltaTime = this->m_stopWatch.Elapsed();
	this->m_stopWatch.Begin();

	if (!this->m_windowHandle)
	{
		return;
	}

	this->m_imguiRenderer.BeginFrame();

	// Variable-timed update:
	this->Update(deltaTime);

	this->Render();

	this->m_composeCommandList->Open();
	{
		auto scopedMarker = this->m_composeCommandList->BeginScopedMarker("Compose back buffer");
		TextureHandle backBuffer = this->m_graphicsDevice->GetBackBuffer();

		this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
		this->m_composeCommandList->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });

		this->m_composeCommandList->SetRenderTargets({ backBuffer }, nullptr);

		this->Compose(this->m_composeCommandList);

		// Render directly to back buffer
		this->m_imguiRenderer.Render(this->m_composeCommandList);

		this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
	}

	this->m_composeCommandList->Close();
	this->m_graphicsDevice->ExecuteCommandLists(this->m_composeCommandList.get());

	this->m_graphicsDevice->Present();
	// Execute Pending Command Lists
}

void Application::Update(TimeStep deltaTime)
{

}

void Application::Render()
{
}

void Application::Compose(PhxEngine::RHI::CommandListHandle cmdList)
{
	// Clear back buffer
}

void PhxEngine::Core::Application::SetWindow(Core::Platform::WindowHandle windowHandle, bool isFullscreen)
{
	assert(this->m_isInitialized);

	// Create Swapchain
	this->m_windowHandle = windowHandle;

	// Create Draw Canvas
	Core::InitializeCanvas(windowHandle, this->m_canvas);

	RHI::SwapChainDesc swapchainDesc = {};
	swapchainDesc.BufferCount = 3;
	swapchainDesc.Format = FormatType::R10G10B10A2_UNORM;
	swapchainDesc.Width = this->m_canvas.GetPhysicalWidth();
	swapchainDesc.Height = this->m_canvas.GetPhysicalHeight();
	swapchainDesc.VSync = true;
	swapchainDesc.WindowHandle = windowHandle;

	this->m_graphicsDevice->CreateSwapChain(swapchainDesc);
	this->m_imguiRenderer.Initialize(this->m_graphicsDevice, this->m_shaderStore, this->m_windowHandle);
}
