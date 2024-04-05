#include "GlfWWindow.h"

#include <PhxEngine/EngineTuner.h>
#include <PhxEngine/Core/Profiler.h>
#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "GlfWWindow.h"

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

PhxEngine::GlfWWindow::GlfWWindow()
{
	PHX_EVENT();

}

PhxEngine::GlfWWindow::~GlfWWindow()
{
	PHX_EVENT();
	this->Finalize();
}

void PhxEngine::GlfWWindow::Finalize()
{
	PHX_EVENT();	
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

void PhxEngine::GlfWWindow::StartUp()
{
	PHX_EVENT();
	this->Initialize();
}

void PhxEngine::GlfWWindow::OnUpdate()
{
	PHX_EVENT();
	glfwPollEvents();
}

void* PhxEngine::GlfWWindow::GetNativeWindowHandle()
{
	return glfwGetWin32Window(this->m_glfwWindow);
}

void PhxEngine::GlfWWindow::Initialize()
{
	PHX_EVENT();
	if (gGlfwNumActiveWindows == 0)
	{
		int success = glfwInit();
		glfwSetErrorCallback(GLFWErrorCallback);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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


	EventBus::TriggerEvent(WindowResizeEvent(this->m_data.Width, this->m_data.Height));
}
