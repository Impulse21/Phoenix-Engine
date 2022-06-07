#include "phxpch.h"

#include "Application.h"

#include "ThirdParty/ImGui/imgui.h"


using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

const char* GraphicsAPIToString(GraphicsAPI api)
{
	switch (api)
	{
	case GraphicsAPI::DX12:
		return "DX12";
	case GraphicsAPI::Unknown:
	default:
		return "UNKNOWN";
	}
}
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

	this->Compose(this->m_composeCommandList);

	this->m_composeCommandList->Close();
	this->m_graphicsDevice->ExecuteCommandLists(this->m_composeCommandList.get());

	this->m_graphicsDevice->Present();
}

void Application::Update(TimeStep deltaTime)
{
	// Set IMGUI Data
	static bool sWindowOpen = true;
	ImGui::Begin("Phx Engine", &sWindowOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Version: %s", "0.1");
	ImGui::Text("Graphics API: %s", GraphicsAPIToString(this->m_graphicsDevice->GetApi()));

	const auto* gpuAdapter = this->m_graphicsDevice->GetGpuAdapter();
	ImGui::Text("Graphics Adapter: %s", gpuAdapter->GetName());
	ImGui::BulletText("Dedicated Video Memory: %zu MB (%zu GB)", gpuAdapter->GetDedicatedVideoMemory() >> 20, gpuAdapter->GetDedicatedVideoMemory() >> 30);
	ImGui::BulletText("Dedicated System Memory: %zu MB (%zu GB)", gpuAdapter->GetDedicatedSystemMemory() >> 20, gpuAdapter->GetDedicatedSystemMemory() >> 30);
	ImGui::BulletText("Shared System Memory: %zu MB (%zu GB)" , gpuAdapter->GetSharedSystemMemory() >> 20, gpuAdapter->GetSharedSystemMemory() >> 30);
	ImGui::End();
	ImGui::ShowDemoWindow();
}

void Application::Render()
{
}

void Application::Compose(PhxEngine::RHI::CommandListHandle cmdList)
{
	auto scopedMarker = this->m_composeCommandList->BeginScopedMarker("Compose back buffer");
	TextureHandle backBuffer = this->m_graphicsDevice->GetBackBuffer();
	
	this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
	this->m_composeCommandList->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });

	this->m_composeCommandList->SetRenderTargets({ backBuffer }, nullptr);

	// Render directly to back buffer
	this->m_imguiRenderer.Render(this->m_composeCommandList);

	this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
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
