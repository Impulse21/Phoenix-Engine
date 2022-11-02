#include "phxpch.h"

#include "PhxEngine/App/Application.h"
#include "PhxEngine/Core/Window.h"
#include "PhxEngine/App/ApplicationEvents.h"
#include "PhxEngine/Systems/ConsoleVarSystem.h"
#include "PhxEngine/Graphics/ShaderStore.h"
#include "PhxEngine/Graphics/ShaderFactory.h"
#include "PhxEngine/Renderer/Initializer.h"

#include "Graphics/ImGui/ImGuiRenderer.h"


#include <imgui.h>

#define ENABLE_FRAME_CAPTURES 0

using namespace PhxEngine;
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

	// Graphics::ShaderFactory factory(this->m_graphicsDevice, "shaders/", "../PhxEngine/Shaders/");
	//this->m_shaderStore.PreloadShaders(factory);

	this->m_beginFrameCommandList = this->m_graphicsDevice->CreateCommandList();
	this->m_composeCommandList = this->m_graphicsDevice->CreateCommandList();

	this->m_frameProfiler = std::make_unique<FrameProfiler>(graphicsDevice, NUM_BACK_BUFFERS);

	this->m_isInitialized = true;
}

void Application::Finalize()
{
	// this->m_imguiRenderer.Finalize();
}

void PhxEngine::Application::Run()
{
	// RUN
}

void Application::Tick()
{
	TimeStep deltaTime = this->m_stopWatch.Elapsed();
	this->m_stopWatch.Begin();

	if (!this->m_windowHandle)
	{
		return;
	}

	this->m_beginFrameCommandList->Open();
	this->m_frameProfiler->BeginFrame(this->m_beginFrameCommandList);
	this->m_beginFrameCommandList->Close();
	this->m_graphicsDevice->ExecuteCommandLists(this->m_beginFrameCommandList.get());

	// this->m_imguiRenderer.BeginFrame();

	// Variable-timed update:
	this->Update(deltaTime);

	this->Render();

	this->m_composeCommandList->Open();

	this->Compose(this->m_composeCommandList);

	this->m_frameProfiler->EndFrame(this->m_composeCommandList);

	this->m_composeCommandList->Close();
	this->m_graphicsDevice->ExecuteCommandLists(this->m_composeCommandList.get());

	this->m_graphicsDevice->Present();
	this->m_frameCount++;
}

void Application::Update(TimeStep deltaTime)
{
	/*
	if (this->m_renderPath)
	{
		this->m_renderPath->Update(deltaTime);
	}
	*/
	// Set IMGUI Data
	static bool sWindowOpen = true;
	ImGui::Begin("Phx Engine", &sWindowOpen);
	ImGui::Indent();
	ImGui::Text("Version: %s", "0.1");
	ImGui::Text("Graphics API: %s", GraphicsAPIToString(this->m_graphicsDevice->GetApi()));

	const auto* gpuAdapter = this->m_graphicsDevice->GetGpuAdapter();
	ImGui::Text("GPU Adapter: %s", gpuAdapter->GetName());

	ImGui::Indent();
	ImGui::BulletText("Video Memory: %zu MB (%zu GB)", gpuAdapter->GetDedicatedVideoMemory() >> 20, gpuAdapter->GetDedicatedVideoMemory() >> 30);
	ImGui::BulletText("System Memory: %zu MB (%zu GB)", gpuAdapter->GetDedicatedSystemMemory() >> 20, gpuAdapter->GetDedicatedSystemMemory() >> 30);
	ImGui::BulletText("Shared System Memory: %zu MB (%zu GB)" , gpuAdapter->GetSharedSystemMemory() >> 20, gpuAdapter->GetSharedSystemMemory() >> 30);
	ImGui::Unindent();
	ImGui::Unindent();

	ImGui::Separator();

	ImGui::Text("Frame Stats");
	ImGui::Indent();

	auto& cpuStats = this->m_frameProfiler->GetCpuTimeStats();
	ImGui::Text("Avg CPU Time: %f ms", cpuStats.GetAvg());
	ImGui::PlotLines("Frame CPU Times", cpuStats.GetExtendedHistory(), static_cast<int>(cpuStats.GetExentedHistorySize()));

	auto& gpuStats = this->m_frameProfiler->GetGpuTimeStats();
	ImGui::Text("Avg GPU Time: %f ms", gpuStats.GetAvg());
	ImGui::PlotLines("Frame CPU Times", gpuStats.GetExtendedHistory(), static_cast<int>(gpuStats.GetExentedHistorySize()));
	
	ImGui::Unindent();

	ImGui::End();

	static bool sConsoleWindowOpen = false;
	ImGui::Begin("Console Variables", &sConsoleWindowOpen);

	ConsoleVarSystem::GetInstance()->DrawImguiEditor();

	ImGui::End();
}

void Application::Render()
{
}

void Application::Compose(PhxEngine::RHI::CommandListHandle cmdList)
{
	/*
	auto scopedMarker = this->m_composeCommandList->BeginScopedMarker("Compose back buffer");
	TextureHandle backBuffer = this->m_graphicsDevice->GetBackBuffer();
	
	this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
	this->m_composeCommandList->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });

	this->m_composeCommandList->SetRenderTargets({ backBuffer }, nullptr);

	if (this->m_renderPath)
	{
		this->m_renderPath->Compose(cmdList);
	}

	// Render directly to back buffer
	this->m_imguiRenderer.Render(this->m_composeCommandList);

	this->m_composeCommandList->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
	*/
}

void Application::SetWindow(Core::Platform::WindowHandle windowHandle, bool isFullscreen)
{
	assert(this->m_isInitialized);

	// Create Swapchain
	this->m_windowHandle = windowHandle;

	// Create Draw Canvas
	Core::InitializeCanvas(windowHandle, this->m_canvas);

	RHI::SwapChainDesc swapchainDesc = {};
	swapchainDesc.BufferCount = NUM_BACK_BUFFERS;
	swapchainDesc.Format = FormatType::R10G10B10A2_UNORM;
	swapchainDesc.Width = this->m_canvas.GetPhysicalWidth();
	swapchainDesc.Height = this->m_canvas.GetPhysicalHeight();
	swapchainDesc.VSync = true;
	swapchainDesc.WindowHandle = windowHandle;
	swapchainDesc.EnableHDR = true;

	this->m_graphicsDevice->CreateSwapChain(swapchainDesc);
	/*
	this->m_imguiRenderer.Initialize(this->m_graphicsDevice, this->m_shaderStore, this->m_windowHandle);

	if (this->m_renderPath)
	{
		this->m_renderPath->OnAttachWindow();
	}
	*/
}


PhxEngine::LayeredApplication::LayeredApplication(ApplicationSpecification const& spec)
	: m_spec(spec)
{
	// Do the Initializtion of the Application sub systems
	LayeredApplication::Ptr = this;
	// Construct Window
	WindowSpecification windowSpec = {};
	windowSpec.Width = this->m_spec.WindowWidth;
	windowSpec.Height = this->m_spec.WindowHeight;
	windowSpec.Title = this->m_spec.Name;
	windowSpec.VSync = this->m_spec.VSync;
	
	this->m_window = WindowFactory::CreateGlfwWindow(windowSpec);
	this->m_window->Initialize();
	this->m_window->SetResizeable(false);
	this->m_window->SetVSync(this->m_spec.VSync);


	// TODO: Create swapchain in Window
	RHI::SwapChainDesc swapchainDesc = {};
	swapchainDesc.BufferCount = NUM_BACK_BUFFERS;
	swapchainDesc.Format = FormatType::R10G10B10A2_UNORM;
	swapchainDesc.Width = this->m_window->GetWidth();
	swapchainDesc.Height = this->m_window->GetHeight();
	swapchainDesc.VSync = true;
	swapchainDesc.WindowHandle = static_cast<Platform::WindowHandle>(m_window->GetNativeWindowHandle());

	IGraphicsDevice::Ptr->CreateSwapChain(swapchainDesc);

	// Prepare Renderer
	this->m_shaderStore = std::make_unique<Graphics::ShaderStore>();
	Graphics::ShaderStore::Ptr = this->m_shaderStore.get();
	Graphics::ShaderFactory factory(IGraphicsDevice::Ptr, "shaders/", "PhxEngine/Shaders/");
	this->m_shaderStore->PreloadShaders(factory);

	this->m_window->SetEventCallback([this](Event& e) {this->OnEvent(e); });

	PhxEngine::Renderer::Initialize();

	this->m_composeCommandList = IGraphicsDevice::Ptr->CreateCommandList();

	if (spec.EnableImGui)
	{
		this->m_imguiRenderer = std::make_shared<Graphics::ImGuiRenderer>();
		this->PushOverlay(this->m_imguiRenderer);
	}
}

PhxEngine::LayeredApplication::~LayeredApplication()
{
	PhxEngine::Renderer::Finalize();
	Graphics::ShaderStore::Ptr = nullptr;
	LayeredApplication::Ptr = nullptr;
}

void PhxEngine::LayeredApplication::Run()
{
	this->OnInit();
	while (this->m_isRunning)
	{
		TimeStep deltaTime = this->m_stopWatch.Elapsed();
		this->m_stopWatch.Begin();

		// Frame counter
		this->m_window->OnUpdate();

		if (!this->m_isMinimized)
		{
			// start CPU timmers
#if ENABLE_FRAME_CAPTURES
			wchar_t wcsbuf[200];
			swprintf(wcsbuf, 200, L"C:\\Users\\dipao\\OneDrive\\Documents\\Pix Captures\\PHXENGINE_CrashFrame_%d.wpix", (int)this->m_frameCount + 1);
			IGraphicsDevice::Ptr->BeginCapture(std::wstring(wcsbuf));
#endif
			// Renderer begin frame
			for (auto& layer : this->m_layerStack)
			{
				// TODO: Time step
				layer->OnUpdate(deltaTime);
			}

			if (this->m_spec.EnableImGui)
			{
				this->RenderImGui();
			}

			this->Render();
			this->Compose();
			
			// Renderer End Frame

			// Execute Renderer on seperate thread maybe?
			// Finish Timers and 
#if ENABLE_FRAME_CAPTURES
			IGraphicsDevice::Ptr->EndCapture();
#endif
			RHI::IGraphicsDevice::Ptr->Present();
			this->m_frameCount++;
		}

		// Calculate step time based on last frame
		IGraphicsDevice::Ptr->WaitForIdle();
	}
}

void PhxEngine::LayeredApplication::PushLayer(std::shared_ptr<AppLayer> layer)
{
	this->m_layerStack.PushLayer(layer);
}

void PhxEngine::LayeredApplication::PushOverlay(std::shared_ptr<AppLayer> layer)
{
	this->m_layerStack.PushOverlay(layer);
}

void PhxEngine::LayeredApplication::OnEvent(Event& e)
{
	if (e.GetEventType() == WindowCloseEvent::GetStaticType())
	{
		this->m_isRunning = false;
		e.IsHandled = true;
	}

	if (e.GetEventType() == WindowResizeEvent::GetStaticType())
	{
		WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
		if (resizeEvent.GetWidth() == 0 && resizeEvent.GetHeight() == 0)
		{
			this->m_isMinimized = true;
		}
		else
		{
			this->m_isMinimized = false;
			// Trigger Resize event on RHI;
		}
	}
}

void PhxEngine::LayeredApplication::RenderImGui()
{
	this->m_imguiRenderer->BeginFrame();

	for (auto& layer : this->m_layerStack)
	{
		layer->OnRenderImGui();
	}
}

void PhxEngine::LayeredApplication::Render()
{
	for (auto& layer : this->m_layerStack)
	{
		layer->OnRender();
	}
}

void PhxEngine::LayeredApplication::Compose()
{
	this->m_composeCommandList->Open();

	{
		auto scopedMarker = this->m_composeCommandList->BeginScopedMarker("Compose back buffer");

		this->m_composeCommandList->BeginRenderPassBackBuffer();
		TextureHandle backBuffer = IGraphicsDevice::Ptr->GetBackBuffer();

		for (auto& layer : this->m_layerStack)
		{
			layer->OnCompose(this->m_composeCommandList);
		}

		this->m_composeCommandList->EndRenderPass();
	}

	this->m_composeCommandList->Close();

	IGraphicsDevice::Ptr->ExecuteCommandLists(this->m_composeCommandList.get());
}
