#include "GltfDisplayService.h"

#include <PhxEngine/EngineTuner.h>
#include <PhxEngine/Core/Profiler.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

using namespace PhxEngine;
namespace
{
	static uint8_t gGlfwNumActiveWindows = 0U;

	static void GLFWErrorCallback(int error, const char* description)
	{
		// HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}
}

IntVar WindowWidth("Engine/Display Service/Window/Height", 2000, 1);
IntVar WindowHeight("Engine/Display Service/Window/Widith", 1200, 1);

void PhxEngine::GltfDisplayService::Startup()
{
	PHX_EVENT();
	if (gGlfwNumActiveWindows == 0)
	{
		int success = glfwInit();
		glfwSetErrorCallback(GLFWErrorCallback);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->m_windowResizeHandler = [this](WindowResizeEvent const& e) {
			RHI::SwapChainDesc desc = {
				.Width = e.Width,
				.Height = e.Height,
			};

			if (RHI::IGfxDevice::Ptr)
			{
				RHI::IGfxDevice::Ptr = RHI::GfxDeviceFactory::Create(RHI::GraphicsAPI::DX12);
				RHI::IGfxDevice::Ptr->Initialize(desc, this->GetNativeWindowHandle());
			}
			else
			{
				RHI::IGfxDevice::Ptr->ResizeSwapchain(desc);
			}
		};

	this->m_data.Width = WindowWidth;
	this->m_data.Height = WindowHeight;

	this->m_glfwWindow = glfwCreateWindow(
		this->m_data.Width,
		this->m_data.Height,
		"PhxEngine",
		nullptr,
		nullptr);

	gGlfwNumActiveWindows++;

	glfwSetWindowUserPointer(this->m_glfwWindow, &this->m_data);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(this->m_glfwWindow, [](GLFWwindow* window, int width, int height) {
			WindowData& Data = *(WindowData*)glfwGetWindowUserPointer(window);
			Data.Width = width;
			Data.Height = height;

			EventBus::TriggerEvent(WindowResizeEvent(width, height));
		});

	glfwSetWindowCloseCallback(this->m_glfwWindow, [](GLFWwindow* window) {
			EventBus::TriggerEvent(WindowCloseEvent());
		});

	glfwSetKeyCallback(this->m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			// TODO: Key Event Dispatching

		});

	EventBus::Subscribe<WindowResizeEvent>(this->m_windowResizeHandler);

	EventBus::TriggerEvent(WindowResizeEvent(this->m_data.Width, this->m_data.Height));
}

void PhxEngine::GltfDisplayService::Shutdown()
{
	PHX_EVENT();
	EventBus::Unsubscribe<WindowResizeEvent>(this->m_windowResizeHandler);

	if (RHI::IGfxDevice::Ptr)
	{
		RHI::IGfxDevice::Ptr->Finalize();
		delete RHI::IGfxDevice::Ptr;
	}
	
	if (this->m_glfwWindow)
	{
		glfwDestroyWindow(this->m_glfwWindow);
		this->m_glfwWindow = nullptr;
		gGlfwNumActiveWindows--;
	}

	if (gGlfwNumActiveWindows == 0)
	{
		glfwTerminate();
	}
}

void PhxEngine::GltfDisplayService::Update()
{
	PHX_EVENT();
	glfwPollEvents();
}

void PhxEngine::GltfDisplayService::Present()
{
	PHX_EVENT();
	RHI::IGfxDevice::Ptr->SubmitFrame();
}

void* PhxEngine::GltfDisplayService::GetNativeWindowHandle()
{
	return glfwGetWin32Window(this->m_glfwWindow);
}
