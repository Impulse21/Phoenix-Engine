#include "phxpch.h"

#include "PhxEngine/Engine/EngineApp.h"
#include "PhxEngine/Core/Window.h"
#include "PhxEngine/Engine/ApplicationEvents.h"
#include "PhxEngine/Systems/ConsoleVarSystem.h"
#include "PhxEngine/Graphics/ShaderStore.h"
#include "PhxEngine/Graphics/ShaderFactory.h"
#include "PhxEngine/Renderer/Initializer.h"
#include <PhxEngine/Renderer/ShaderCodeLibrary.h>

#include "Graphics/ImGui/ImGuiRenderer.h"

#include <imgui.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Renderer;


static AutoConsoleVar_Int sPixCapture("Debug.EnablePixCapture", "PixCapture", 0, ConsoleVarFlags::EditCheckbox);

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

PhxEngine::EngineApp::EngineApp(ApplicationSpecification const& spec)
	: m_spec(spec)
{
	// Do the Initializtion of the Application sub systems
	assert(GPtr);
	EngineApp::GPtr = this;

}

PhxEngine::EngineApp::~EngineApp()
{
	PhxEngine::Renderer::Finalize();
	Graphics::ShaderStore::GPtr = nullptr;
	EngineApp::GPtr = nullptr;
}

void PhxEngine::EngineApp::PreInitialize()
{
	// Construct Window
	WindowSpecification windowSpec = {};
	windowSpec.Width = this->m_spec.WindowWidth;
	windowSpec.Height = this->m_spec.WindowHeight;
	windowSpec.Title = this->m_spec.Name;
	windowSpec.VSync = this->m_spec.VSync;

	// this->m_window = WindowFactory::CreateGlfwWindow(windowSpec);
	this->m_window->Initialize();
	this->m_window->SetResizeable(false);
	this->m_window->SetVSync(this->m_spec.VSync);

	// TODO: Create swapchain in Window
	RHI::RHIViewportDesc viewportDesc = {};
	viewportDesc.BufferCount = NUM_BACK_BUFFERS;
	viewportDesc.Format = FormatType::R10G10B10A2_UNORM;
	viewportDesc.Width = this->m_window->GetWidth();
	viewportDesc.Height = this->m_window->GetHeight();
	viewportDesc.VSync = false;
	viewportDesc.WindowHandle = static_cast<Platform::WindowHandle>(m_window->GetNativeWindowHandle());

	// Render Something
	this->m_viewport = GRHI->CreateViewport(viewportDesc);

	// Initialize Loading scene TODO: Let application override render logic for custom screens.

	// GRHI->CreateShader();

}

void PhxEngine::EngineApp::Initialize()
{
	// Prepare Renderer
	this->m_shaderStore = std::make_unique<Graphics::ShaderStore>();
	Graphics::ShaderStore::GPtr = this->m_shaderStore.get();
	Graphics::ShaderFactory factory(IGraphicsDevice::GPtr, "shaders/", "PhxEngine/Shaders/");
	this->m_shaderStore->PreloadShaders(factory);

	this->m_window->SetEventCallback([this](Event& e) {this->OnEvent(e); });

	PhxEngine::Renderer::Initialize();

	if (this->m_spec.EnableImGui)
	{
		this->m_imguiRenderer = std::make_shared<Graphics::ImGuiRenderer>();
		this->PushOverlay(this->m_imguiRenderer);
	}
	this->OnInit();
}

void PhxEngine::EngineApp::Run()
{
	this->Initialize();
	while (this->m_isRunning)
	{
		TimeStep deltaTime = this->m_stopWatch.Elapsed();
		this->m_stopWatch.Begin();

		// Frame counter
		this->m_window->OnUpdate();

		if (!this->m_isMinimized)
		{
			// start CPU timmers
			if ((bool)sPixCapture.Get())
			{
				wchar_t wcsbuf[200];
				swprintf(wcsbuf, 200, L"C:\\Users\\dipao\\OneDrive\\Documents\\Pix Captures\\PhxEngine_Debug\\PHXENGINE_CrashFrame_%d.wpix", (int)this->m_frameCount + 1);
				IGraphicsDevice::GPtr->BeginCapture(std::wstring(wcsbuf));
			}

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

			if ((bool)sPixCapture.Get())
			{
				// Only capture if the Device was removed
				// bool isRemoved = IGraphicsDevice::Ptr->IsDevicedRemoved();
				IGraphicsDevice::GPtr->EndCapture();
			}

			// Execute Renderer on seperate thread maybe?
			// Finish Timers and 

			RHI::IGraphicsDevice::GPtr->Present();
			this->m_frameCount++;
		}

		// Calculate step time based on last frame
		IGraphicsDevice::GPtr->WaitForIdle();
	}
}

void PhxEngine::EngineApp::PushLayer(std::shared_ptr<AppLayer> layer)
{
	this->m_layerStack.PushLayer(layer);
}

void PhxEngine::EngineApp::PushOverlay(std::shared_ptr<AppLayer> layer)
{
	this->m_layerStack.PushOverlay(layer);
}

void PhxEngine::EngineApp::OnEvent(Event& e)
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

void PhxEngine::EngineApp::RenderImGui()
{
	this->m_imguiRenderer->BeginFrame();

	for (auto& layer : this->m_layerStack)
	{
		layer->OnRenderImGui();
	}
}

void PhxEngine::EngineApp::Render()
{
	for (auto& layer : this->m_layerStack)
	{
		layer->OnRender();
	}
}

void PhxEngine::EngineApp::Compose()
{
	/*
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
	*/
}
