#include "OSWindow.h"

#include <PhxEngine/Core/Pool.h>

#if defined(PHX_PLATFORM_WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(PHX_PLATFORM_LINUX)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <atomic>

using namespace phx;
using namespace phx::platform;

namespace
{
    struct OSWindowImpl
    {
        GLFWwindow* glfw_window;
        WindowDescriptor desc;
    };

    SmallObjectPool<OSWindow, OSWindowImpl, 16> g_window_pool;
}

OSWindowHandle phx::platform::CreateOSWindow(const WindowDescriptor& desc)
{
  return OSWindowHandle();
}

void phx::platform::DestroyOSWindow(OSWindowHandle handle) {}

bool phx::platform::PollEvents()
{
  return false;
}

void* phx::platform::GetNativeHandle(OSWindowHandle handle)
{
  return nullptr;
}
