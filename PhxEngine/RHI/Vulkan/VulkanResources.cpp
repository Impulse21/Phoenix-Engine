#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/OSWindow.h>

#include "RHIVulkan.h"
#include "RHIVulkanResources.h"

// -- Viewport platform specific includes ---
#if defined(PHX_PLATFORM_WINDOWS)

#define NOMINMAX
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

namespace 
{
    struct SurfaceCapabilities
    {
         VkSurfaceCapabilitiesKHR vk_capabilities = {};
         std::vector<VkSurfaceFormatKHR> vk_formats;
         std::vector<VkPresentModeKHR> vk_present_modes;
    }
}

static VkSurfaceKHR CreateSurface(void* window_native_handle);
static void QuerySurfaceCapabilities(SurfaceCapabilities& cap);
static VkSurfaceFormatKHR SelectSurfaceFormat(const SurfaceCapabilities& capabilities, bool hdr, VkFormat desired_format);

ViewportHandle phx::rhi::CreateViewport(const ViewportDesc& desc)
{
    ViewportHandle viewport_handle = g_context.pool_viewports.Allocate();
    vulkan::ViewportImpl* viewport = g_context.pool_viewports.Get(viewport_handle);

    void* window_native_handle = phx::platform::GetNativeHandle(desc.window_handle);
    viewport->vk_surface = CreateSurface(window_native_handle);

    SurfaceCapabilities capabilities;
    QuerySurfaceCapabilities(
        g_context.vk_physical_device,
        viewport->vk_surface,
        capabilities);
    
    VkSurfaceFormatKHR surface_format = 
        SelectSurfaceFormat(capabilities, desc.enable_hdr, FormatToVkFormat(desc.format));
    

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!desc.v_sync)
    {
        for (VkPresentModeKHR mode : capabilities.vk_present_modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        if (present_mode != VK_PRESENT_MODE_MAILBOX_KHR)
            PHX_LOG_INFO(Log::Channels::RHI, "Mailbox not available — falling back to FIFO");
    }

    uint32_t image_count = (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) ? 3 : 2;
    image_count = std::max(image_count, capabilities.vk_capabilities.minImageCount);
    if (capabilities.vk_capabilities.maxImageCount > 0)
        image_count = std::min(image_count, capabilities.vk_capabilities.maxImageCount);

    return viewport_handle;
}

void phx::rhi::DestoryViewport(ViewportHandle handle)
{

}


static VkSurfaceFormatKHR SelectSurfaceFormat(const SurfaceCapabilities& capabilities, bool enable_hdr, VkFormat format)
{
    VkFormat         desired_format      = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR  desired_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    if (enable_hdr)
    {
        // HDR — 10-bit per channel, PQ transfer function.
        desired_format      = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        desired_color_space = VK_COLOR_SPACE_HDR10_ST2084_EXT;
    }
    else
    {
        desired_format = format;
        desired_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }

    for (const auto& fmt : capabilities.vk_formats)
    {
        if (fmt.format == desired_format && fmt.colorSpace == desired_color_space)
            return fmt;
    }

    // HDR colour space not found — fall back to SDR rather than picking a
    // wrong colour space silently.
    if (enable_hdr)
    {
        PHX_LOG_WARN(Log::Channels::RHI, "HDR surface format not available — falling back to SDR");
        for (const auto& fmt : capabilities.vk_formats)
        {
            if (fmt.format     == VK_FORMAT_B8G8R8A8_UNORM &&
                fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return fmt;
        }
    }

    // Driver default — spec guarantees at least one.
    PHX_LOG_WARN(Log::Channels::RHI, "Desired surface format not available — using driver default");
    return capabilities.vk_formats[0];
}

static void QuerySurfaceCapabilities(
    VkPhysicalDevice vk_physical_device,
    VkSurfaceKHR vk_surface,
    SurfaceCapabilities& out)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk_physical_device,
        vk_surface,
        &out.vk_capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk_physical_device,
        vk_surface,
        &format_count, nullptr);

    out.vk_formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk_physical_device,
        vk_surface,
        &format_count, out.vk_formats.data());

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk_physical_device,
        vk_surface,
        &mode_count, nullptr);

    out.vk_present_modes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk_physical_device,
        vk_surface,
        &mode_count, out.vk_present_modes.data());
}

static VkSurfaceKHR CreateSurface(void* window_native_handle)
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