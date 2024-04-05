#include <PhxEngine/Core/Application.h>
#include <PhxEngine/Core/StopWatch.h>


using namespace PhxEngine;

Layer::Layer(const std::string& debugName)
	: m_debugName(debugName)
{
}

LayerStack::~LayerStack()
{
	for (Layer* layer : this->m_layers)
	{
		layer->OnDetach();
		phx_delete(layer);
	}
}

void LayerStack::PushLayer(Layer* layer)
{
	this->m_layers.emplace(this->m_layers.begin() + this->m_layerInsertIndex, layer);
	this->m_layerInsertIndex++;
}

void LayerStack::PushOverlay(Layer* overlay)
{
	this->m_layers.emplace_back(overlay);
}

void LayerStack::PopLayer(Layer* layer)
{
	auto it = std::find(this->m_layers.begin(), this->m_layers.begin() + this->m_layerInsertIndex, layer);
	if (it != this->m_layers.begin() + this->m_layerInsertIndex)
	{
		layer->OnDetach();
		this->m_layers.erase(it);
		this->m_layerInsertIndex--;
	}
}

void LayerStack::PopOverlay(Layer* overlay)
{
	auto it = std::find(this->m_layers.begin() + this->m_layerInsertIndex, this->m_layers.end(), overlay);
	if (it != this->m_layers.end())
	{
		overlay->OnDetach();
		this->m_layers.erase(it);
	}
}


PhxEngine::Application::Application()
{
	PHX_CORE_ASSERT(!s_singleton, "Application already exists!");
	s_singleton = this;


	this->m_windowResizeHandler = [this](WindowResizeEvent const& e) {
		RHI::SwapChainDesc desc = {
			.Width = e.Width,
			.Height = e.Height,
		};

		if (this->m_gfxDevice == nullptr)
		{
			this->m_gfxDevice = std::unique_ptr<RHI::GfxDevice>(RHI::GfxDeviceFactory::Create(RHI::GraphicsAPI::DX12));
			this->m_gfxDevice->Initialize(desc, this->m_window->GetNativeWindowHandle());

			RHI::IGfxDevice::Ptr = this->m_gfxDevice.get();
		}
		else
		{
			this->m_gfxDevice->ResizeSwapchain(desc);
		}
	};

	EventBus::Subscribe<WindowResizeEvent>(this->m_windowResizeHandler);
	EventBus::Subscribe<WindowCloseEvent>([this](WindowCloseEvent const& e) { this->m_running = false; });
	this->m_window = WindowFactory::Create();
	this->m_window->StartUp();
}

PhxEngine::Application::~Application()
{
	this->m_gfxDevice->WaitForIdle();
	RHI::GfxDevice::Ptr = nullptr;

	// DO shutdown;
}

void PhxEngine::Application::PushLayer(Layer* layer)
{
	this->m_layerStack.PushLayer(layer);
	layer->OnAttach();
}

void PhxEngine::Application::PushOverlay(Layer* layer)
{
	this->m_layerStack.PushOverlay(layer);
	layer->OnAttach();
}

void PhxEngine::Application::Run()
{
	StopWatch frameClock;
	while (this->m_running)
	{
		PHX_FRAME("MainThread");

		TimeStep deltaTime = frameClock.Elapsed();
		this->m_lastFrameTime = deltaTime;

		this->m_window->OnUpdate();
		if (!this->m_minimized)
		{
			{
				PHX_EVENT("LayerStack OnUpdate");

				for (Layer* layer : this->m_layerStack)
					layer->OnUpdate(deltaTime);
			}

#if false
			m_ImGuiLayer->Begin();
			{
				HZ_PROFILE_SCOPE("LayerStack OnImGuiRender");

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();
#endif
		}

		this->m_gfxDevice->SubmitFrame();
	}
}