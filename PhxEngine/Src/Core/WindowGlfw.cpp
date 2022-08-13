#include "D:/Users/C.DiPaolo/Development/Phoenix-Engine/build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "phxpch.h"
#include "WindowGlfw.h"

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "PhxEngine/App/ApplicationEvents.h"

using namespace PhxEngine::Core;

namespace
{
	static uint8_t gGlfwNumActiveWindows = 0U;

	static void GLFWErrorCallback(int error, const char* description)
	{
		// HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}
}

WindowGlfw::WindowGlfw(WindowSpecification const& spec)
	: m_spec(spec)
	, m_glfwWindow(nullptr)
{
}

PhxEngine::Core::WindowGlfw::~WindowGlfw()
{

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

void PhxEngine::Core::WindowGlfw::Initialize()
{
	if (gGlfwNumActiveWindows == 0)
	{
		int success = glfwInit();
		glfwSetErrorCallback(GLFWErrorCallback);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	this->m_glfwWindow = glfwCreateWindow(
		static_cast<int>(this->m_spec.Width),
		static_cast<int>(this->m_spec.Height),
		m_spec.Title.c_str(),
		nullptr,
		nullptr);
	
	gGlfwNumActiveWindows++;

	this->m_data.Width = this->m_spec.Width;
	this->m_data.Height = this->m_spec.Height;

	glfwSetWindowUserPointer(this->m_glfwWindow, &this->m_data);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(this->m_glfwWindow, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

	glfwSetWindowCloseCallback(this->m_glfwWindow, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});
}

void PhxEngine::Core::WindowGlfw::OnUpdate()
{
	glfwPollEvents();
}

void PhxEngine::Core::WindowGlfw::SetResizeable(bool isResizeable)
{
	this->m_isResizeable = isResizeable;
	glfwWindowHint(GLFW_RESIZABLE, this->m_isResizeable);

}

bool PhxEngine::Core::WindowGlfw::IsResizeable() const
{
	return this->m_isResizeable;
}

void PhxEngine::Core::WindowGlfw::Maximize()
{
	glfwMaximizeWindow(this->m_glfwWindow);
}

void PhxEngine::Core::WindowGlfw::CentreWindow()
{
	// todo
}

void* PhxEngine::Core::WindowGlfw::GetNativeWindowHandle()
{
	return glfwGetWin32Window(this->m_glfwWindow);
}
