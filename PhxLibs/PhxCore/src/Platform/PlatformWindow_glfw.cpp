#include "PhxCore_pch.h"

#include <PhxCore/Platform/PlatformWindow.h>
#include <PhxCore/Pool.h>

#if defined(PHX_PLATFORM_WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(PHX_PLATFORM_LINUX)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

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
        phx::Result<WindowHandle> CreateWindowInstance(const WindowDescriptor& desc)
        {
            if (g_window_pool.GetCount() == 0)
            {
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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


        std::pair<uint32_t, uint32_t> GetWindowSize(WindowHandle handle)
        {
            PHX_ASSERT(
                g_window_pool.Contains(handle),
                "Invalid window handle passed to GetWindowSize!");

            WindowImpl *impl = g_window_pool.Get(handle);

            int width, height;
            glfwGetWindowSize(impl->glfw_window, &width, &height);
            return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        }

        void DestroyWindowInstance(WindowHandle handle)
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
            PHX_ASSERT(
                g_window_pool.Contains(handle),
                "Invalid window handle passed to PollEvents!");

            WindowImpl *impl = g_window_pool.Get(handle);

            glfwPollEvents();
            return !glfwWindowShouldClose(impl->glfw_window);
        }


        window_native_handle GetNativeHandle(WindowHandle handle) 
        {
            PHX_ASSERT(
                g_window_pool.Contains(handle),
                "Invalid window handle passed to GetNativeHandle!");

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

