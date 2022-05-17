#include "phxpch.h"

#include "Application.h"

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

void Application::Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice)
{
	this->m_graphicsDevice = graphicsDevice;
	this->m_composeCommandList = this->m_graphicsDevice->CreateCommandList();

	// TODO: Initalize Sub Systems
	this->m_isInitialized = true;
}

void Application::Finalize()
{

}

void Application::RunFrame()
{
	// TODO: Initialization?
	TimeStep deltaTime = this->m_stopWatch.Elapsed();
	this->m_stopWatch.Begin();

	// Variable-timed update:
	this->Update(deltaTime);

	this->Render();

	this->m_composeCommandList->Open();
	{
		auto scopedMarker = this->m_composeCommandList->BeginScopedMarker("Compose back buffer");
		TextureHandle backBuffer = this->m_graphicsDevice->GetBackBuffer();

		this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
		this->m_composeCommandList->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });

		this->Compose(this->m_composeCommandList);

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

}
