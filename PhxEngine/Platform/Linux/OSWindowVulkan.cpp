#include <PhxEngine/Platform/OSWindowVulkan.h>

#include <GLFW/glfw3.h>

namespace phx::platform
{
    bool vulkan::CreateSurface(VkInstance instance, platform::OSWindowHandle handle, VkSurfaceKHR* out_surface)
    {
        void* window_native_handle = phx::platform::GetNativeHandle(handle);

        GLFWwindow* w = static_cast<GLFWwindow*>(GetNativeHandle(handle));

        VkResult result = glfwCreateWindowSurface(instance, w, nullptr, out_surface);

        return result == VK_SUCCESS;
    }
}