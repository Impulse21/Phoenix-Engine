#include <PhxEngine/Platform/OSWindow.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Pool.h>

  #define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <atomic>

using namespace phx;
using namespace phx::platform;

namespace
{
    struct WaylandHandles
    {
        wl_display* display = nullptr;
        wl_surface* surface = nullptr;
    };

    struct OSWindowImpl
    {
        GLFWwindow* glfw_window;
        WaylandHandles native_handles;
        WindowDescriptor desc;
    };

    SmallObjectPool<OSWindow, OSWindowImpl, 16> g_window_pool;
}

OSWindowHandle phx::platform::CreateOSWindow(const WindowDescriptor& desc)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (!glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
    {
        PHX_LOG_ERROR(Log::Channels::Platform, "Wayland is not supported by this GLFW build. Falling back to default.");
        return {};
    }

    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

    if (g_window_pool.GetCount() == 0)
    {
        if (!glfwInit())
        {
            PHX_LOG_ERROR(Log::Channels::Platform, "Failed to initialize GLFW");
            return {};
        }
    }

    GLFWwindow* glfw_window = glfwCreateWindow(static_cast<int>(desc.width),
                                               static_cast<int>(desc.height),
                                               desc.title, nullptr, nullptr);

    if (!glfw_window)
    {
        PHX_LOG_ERROR(Log::Channels::Engine, "Failed to create GLFW window");
        return {};
    }

    OSWindowHandle handle = g_window_pool.Allocate();
    PHX_ASSERT(handle.IsValid());

    OSWindowImpl* impl = g_window_pool.Get(handle);
    impl->glfw_window = glfw_window;
    impl->native_handles.display = glfwGetWaylandDisplay();
    impl->native_handles.surface = glfwGetWaylandWindow(impl->glfw_window);
    impl->desc = desc;

    return handle;
}

void phx::platform::DestroyOSWindow(OSWindowHandle handle)
{
    if (!g_window_pool.Contains(handle))
    {
        return;
    }

    OSWindowImpl* impl = g_window_pool.Get(handle);

    glfwDestroyWindow(impl->glfw_window);
    g_window_pool.Free(handle);

    if (g_window_pool.GetCount() == 0) 
      glfwTerminate();
}

void phx::platform::PollEvents()
{
    glfwPollEvents();
}

bool phx::platform::ShouldClose(OSWindowHandle handle)
{
    PHX_ASSERT(g_window_pool.Contains(handle));

    OSWindowImpl* impl = g_window_pool.Get(handle);

    glfwPollEvents();
    return !glfwWindowShouldClose(impl->glfw_window);
}

void* phx::platform::GetNativeHandle(OSWindowHandle handle)
{
    PHX_ASSERT(g_window_pool.Contains(handle));

    OSWindowImpl* impl = g_window_pool.Get(handle);
    return &impl->native_handles;
}
