#include <PhxEngine/Platform/OSWindowVulkan.h>

#include <PhxEngine/Core/Log.h>

#include <GLFW/glfw3.h>

namespace phx::platform
{
    bool vulkan::CreateSurface(VkInstance instance, platform::OSWindowHandle handle, VkSurfaceKHR* out_surface)
    {
        GLFWwindow* w = static_cast<GLFWwindow*>(GetNativeHandle(handle));

        PHX_ASSERT(instance != VK_NULL_HANDLE);
        PHX_ASSERT(w);

        PHX_LOG_INFO(
            phx::Log::Channels::Platform,
            "glfwCreateWindowSurface for GLFWwindow* = {0}",
            (void*)w);
        VkResult result = glfwCreateWindowSurface(instance, w, nullptr, out_surface);

        return result == VK_SUCCESS;
    }
}