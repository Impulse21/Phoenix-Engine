#include "PhxCore_pch.h"

#include <PhxCore/Platform/PlatformWindow.h>
#include <PhxCore/Pool.h>

#define GLFW_EXPOSE_NATIVE_WAYLAND

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <atomic>

using namespace phx;


namespace
{
    struct WindowImpl
    {
        GLFWwindow* glfw_window;
        WindowDescriptor desc;
    };

    SmallObjectPool<Window, WindowImpl, 16> g_window_pool;
}

namespace phx
{

    namespace Platform
    {
        phx::Result<WindowHandle> CreateWindow(const WindowDescriptor& desc)
        {
            if (g_window_pool.GetCount() == 0)
            {
#ifdef PHX_PLATFORM_LINUX
                if (!glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
                {
                    PHX_CORE_ERROR("Wayland is not supported by this GLFW build. Falling back to default.");
                    return Unexpected(ResultError::Failure);
                }

                glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#endif 
                if (!glfwInit())
                    return Unexpected(ResultError::Failure);   
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            GLFWwindow* glfw_window = 
                glfwCreateWindow(
                    static_cast<int>(desc.width),
                    static_cast<int>(desc.height),
                    desc.title,
                    nullptr,
                    nullptr);


            if (!glfw_window)
                return Unexpected(ResultError::Failure);

            WindowHandle handle = g_window_pool.Allocate();
            PHX_ASSERT(handle.IsValid(), "Failed to allocate window handle from pool!");
            
            WindowImpl* impl = g_window_pool.Get(handle);
            impl->glfw_window = glfw_window;
            impl->desc = desc;

            return handle;
        }

        void DestroyWindow(WindowHandle handle)
        {
            if (!g_window_pool.Contains(handle))
                return;

            WindowImpl *impl = g_window_pool.Get(handle);
            
            glfwDestroyWindow(impl->glfw_window);
            g_window_pool.Free(handle);
            
            if (g_window_pool.GetCount() == 0)
                glfwTerminate();
        }

        bool PollEvents(WindowHandle handle)
        { 
            if (!g_window_pool.Contains(handle))
                return;

            WindowImpl *impl = g_window_pool.Get(handle);

            glfwPollEvents();
            return !glfwWindowShouldClose(impl->glfw_window);
        }


        window_native_handle GetNativeHandle(WindowHandle handle) 
        {
            if (!g_window_pool.Contains(handle))
                return;

            WindowImpl *impl = g_window_pool.Get(handle);

#if defined(PHX_PLATFORM_WINDOWS)

            return glfwGetWin32Window(impl->glfw_window);

#elif defined(PHX_PLATFORM_LINUX)  

            WaylandHandles wayland_handles = {

                .display = glfwGetWaylandDisplay(),
                .surface = glfwGetWaylandWindow(impl->glfw_window)
            };
            
            return wayland_handles;

#else
            #error "Unsupported platform"
#endif
        }
    }
}

