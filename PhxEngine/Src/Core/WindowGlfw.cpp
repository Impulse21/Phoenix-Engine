#include <phxpch.h>
#include "WindowGlfw.h"

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "PhxEngine/Engine/ApplicationEvents.h"
#include "PhxEngine/Engine/EngineRoot.h"

using namespace PhxEngine::Core;

namespace
{
	static uint8_t gGlfwNumActiveWindows = 0U;

	static void GLFWErrorCallback(int error, const char* description)
	{
		// HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}
}

WindowGlfw::WindowGlfw(PhxEngine::IPhxEngineRoot* engRoot, WindowSpecification const& spec)
	: m_root(engRoot)
	, m_spec(spec)
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
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
			WindowData& Data = *(WindowData*)glfwGetWindowUserPointer(window);
			Data.Width = width;
			Data.Height = height;

			WindowResizeEvent event(width, height);
			if (Data.EventCallback)
			{
				Data.EventCallback(event);
			}
		});

	glfwSetWindowCloseCallback(this->m_glfwWindow, [](GLFWwindow* window)
		{
			WindowData& Data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			if (Data.EventCallback)
			{
				Data.EventCallback(event);
			}
		});

	glfwSetKeyCallback(this->m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
		WindowKeyEvent event(key, scancode, action, mods);
		if (data.EventCallback)
		{
			data.EventCallback(event);
		}
			
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

bool PhxEngine::Core::WindowGlfw::ShouldClose()
{
	return glfwWindowShouldClose(this->m_glfwWindow);
}

void PhxEngine::Core::WindowGlfw::SetWindowTitle(std::string_view strView)
{
	if (strView == this->m_windowTitle)
	{
		return;
	}

	glfwSetWindowTitle(this->m_glfwWindow, strView.data());

	this->m_windowTitle = strView;
}
