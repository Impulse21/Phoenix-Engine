#include <PhxEngine/RHI/RHI.h>

{
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;

#if defined(PHX_PLATFORM_WINDOWS)

    HWND h_window = static_cast<HWND>(window_native_handle);
    VkWin32SurfaceCreateInfoKHR surface_ci
    {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd      = h_window,
    };

    vulkan_check(
        vkCreateWin32SurfaceKHR(
            g_context.vk_instance,
            &surface_ci,
            nullptr,
            &vk_surface));

#elif defined(PHX_PLATFORM_LINUX)

    struct WaylandHandles
    {
        wl_display* display = nullptr;
        wl_surface* surface = nullptr;
    };
    WaylandHandles* wayland_handle = reinterpret_cast<WaylandHandles*>(window_native_handle);
    VkWaylandSurfaceCreateInfoKHR surface_ci
    {
        .sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = wayland_handle->display,
        .surface = wayland_handle->surface,
    };

    vulkan_check(
        vkCreateWaylandSurfaceKHR(
            g_context.vk_instance,
            &surface_ci,
            nullptr,
            &vk_surface));
#endif

    return vk_surface;
}