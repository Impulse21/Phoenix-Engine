#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/OSWindow.h>
#include <PhxEngine/Platform/OSWindowVulkan.h>

#include "RHIVulkan.h"
#include "RHIVulkanResources.h"

#include <algorithm>

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
    };
}

static void QuerySurfaceCapabilities(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface, SurfaceCapabilities& out);
static VkSurfaceFormatKHR SelectSurfaceFormat(const SurfaceCapabilities& capabilities, bool hdr, VkFormat desired_format);

ViewportHandle phx::rhi::CreateViewport(const ViewportDesc& desc)
{
    ViewportHandle viewport_handle = g_context.pool_viewports.Allocate();
    vulkan::ViewportImpl* viewport = g_context.pool_viewports.Get(viewport_handle);
    
    PHX_ASSERT(g_context.vk_instance != VK_NULL_HANDLE);
    PHX_ASSERT(desc.window_handle.IsValid());
     if (!platform::vulkan::CreateSurface(g_context.vk_instance, desc.window_handle, &viewport->vk_surface))
     {
        PHX_LOG_ERROR(Log::Channels::RHI, "Failed to create surface from platform layer.");
        PHX_ASSERT(false);
        std::abort();

        return {};
     }

    SurfaceCapabilities capabilities;
    QuerySurfaceCapabilities(
        g_context.vk_physical_device,
        viewport->vk_surface,
        capabilities);

    VkExtent2D extent = {};
    if (capabilities.vk_capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.vk_capabilities.currentExtent;
    }
    else
    {
        extent.width  = std::clamp(
            desc.width,
            capabilities.vk_capabilities.minImageExtent.width,
            capabilities.vk_capabilities.maxImageExtent.width);

        extent.height = std::clamp(
            desc.height,
            capabilities.vk_capabilities.minImageExtent.height,
            capabilities.vk_capabilities.maxImageExtent.height);
    }

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

    // --- Create swapchain ----------------------------------------------------
    VkSwapchainCreateInfoKHR swapchain_ci
    {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = viewport->vk_surface,
        .minImageCount    = image_count,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,   // present == graphics family
        
        // Handles screen rotation, almost entirely a mobile/tablet concern. 
        // On desktop this is basically always VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 
        .preTransform     = capabilities.vk_capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = present_mode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,    // pass previous handle here on resize
    };

    vulkan_check(
        vkCreateSwapchainKHR(
            g_context.vk_device,
            &swapchain_ci,
            nullptr,
            &viewport->vk_swapchain));

    viewport->vk_swapchain_image_format = surface_format.format;

    uint32_t actual_image_count = 0;
    vkGetSwapchainImagesKHR(g_context.vk_device, viewport->vk_swapchain, &actual_image_count, nullptr);
    
    viewport->vk_images = std::make_unique<VkImage[]>(actual_image_count);
    viewport->vk_image_views = std::make_unique<VkImageView[]>(actual_image_count);
    viewport->image_count = actual_image_count;
    
    vkGetSwapchainImagesKHR(
        g_context.vk_device,
        viewport->vk_swapchain,
        &actual_image_count,
        viewport->vk_images.get());

    for (uint32_t i = 0; i < actual_image_count; ++i)
    {
        VkImageViewCreateInfo view_ci
        {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = viewport->vk_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = viewport->vk_swapchain_image_format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };

        vulkan_check(
            vkCreateImageView(
                g_context.vk_device,
                &view_ci,
                nullptr,
                &viewport->vk_image_views[i]));
    }

    // -- construct semaphores ---
    viewport->vk_image_available_sem = std::make_unique<VkSemaphore[]>(viewport->image_count);
    viewport->vk_render_finished_sem = std::make_unique<VkSemaphore[]>(viewport->image_count);
    
    VkSemaphoreCreateInfo sem_ci { 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
    };

    for (uint32_t i = 0; i < viewport->image_count; ++i)
    {
        vulkan_check(vkCreateSemaphore(g_context.vk_device, &sem_ci, nullptr, &viewport->vk_image_available_sem[i]));
        vulkan_check(vkCreateSemaphore(g_context.vk_device, &sem_ci, nullptr, &viewport->vk_render_finished_sem[i]));
    }

    return viewport_handle;
}

void phx::rhi::DestoryViewport(ViewportHandle handle)
{
    g_context.deferred_callback_queue.EnqueueDelete({
        .frame = g_context.frame_number, 
        .deferred_func = [handle]() {
            auto viewport = g_context.pool_viewports.Get(handle);
            if (!viewport)
                return;

            for (u64 i = 0; i < viewport->image_count; ++i)
            {
                vkDestroyImageView(g_context.vk_device, viewport->vk_image_views[i], nullptr);
                vkDestroySemaphore(g_context.vk_device, viewport->vk_image_available_sem[i], nullptr);
                vkDestroySemaphore(g_context.vk_device, viewport->vk_render_finished_sem[i], nullptr);
            }
            
            vkDestroySwapchainKHR(g_context.vk_device, viewport->vk_swapchain, nullptr);
            vkDestroySurfaceKHR(g_context.vk_instance, viewport->vk_surface, nullptr);

            g_context.pool_viewports.Free(handle);
        }
    });
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