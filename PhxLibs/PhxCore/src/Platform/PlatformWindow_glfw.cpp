#include "PhxCore_pch.h"

#include <PhxCore/Platform/PlatformWindow.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <atomic>
using namespace phx;


namespace
{
    uint8_t g_glfw_ref_counter = 0;
}
namespace phx
{
    struct Window_T
    {
        GLFWindow* glfw_window;
        WindowDescriptor desc;
    };

    namespace Platform
    {
        phx::Result<Window> CreateWindow(const WindowDescriptor& desc)
        {
            if (g_glfw_ref_counter == 0)
            {
#ifdef PHX_PLATFORM_LINUX
                if (!glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
                {
                    PHX_CORE_ERROR("Wayland is not supported by this GLFW build. Falling back to default.");
                    return Unexpected(ResultError::Failure);
                }

                glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#endif 
                if (glfwInit())
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

            g_glfw_ref_counter++;

            Window handle = new Window_T();
            handle->glfw_window = glfw_window;
            handle->desc = desc;

            return handle;
        }

        void DestroyWindow(Window handle)
        {
            glfwDestroyWindow(handle->glfw_window);
            delete handle;

            g_glfw_ref_counter--;
            if (g_glfw_ref_counter == 0)
                glfwTerminate();
        }

        bool PollEvents(Window handle)
        { 
            glfwPollEvents();
            return !glfwWindowShouldClose(handle->glfw_window);
        }


        window_native_handle GetNativeHandle(Window handle) 
        {
#if defined(PHX_PLATFORM_WINDOWS)

            return glfwGetWin32Window(w->glfw_window);

#elif defined(PHX_PLATFORM_LINUX)  

            WaylandHandles wayland_handles = {

                .display = glfwGetWaylandDisplay(),
                .surface = glfwGetWaylandWindow(handle->glfw_window)
            };
            
            return wayland_handles;

#else
            #error "Unsupported platform"
#endif
        }
    }
}

