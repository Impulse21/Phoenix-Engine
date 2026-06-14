#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/OSWindow.h>

#include "RHIVulkan.h"
#include "RHIVulkanResources.h"

// -- Viewport platform specific includes ---
#if defined(PHX_PLATFORM_WINDOWS)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h> // For GetModuleHandle

#else

#include <wayland-client.h>
#endif


using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

ViewportHandle phx::rhi::CreateViewport(const ViewportDesc& desc)
{
    g_context.resource_pools.viewports
    VkSurfaceKHR vk_surface;
    void* window_native_handle = phx::platform::GetNativeHandle(desc.window_handle);
#if defined(PHX_PLATFORM_WINDOWS)

    HWND h_window = static_cast<HWND>(window_native_handle);
    VkWin32SurfaceCreateInfoKHR surface_ci
    {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = static_cast<HINSTANCE>(h_window),
        .hwnd      = static_cast<HWND>(window.hwnd),
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


    return true;
}

void phx::rhi::DestoryViewport(ViewportHandle handle)
{

}