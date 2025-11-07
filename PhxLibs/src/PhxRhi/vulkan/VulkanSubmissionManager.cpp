#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanSubmissionManager.h"

bool CreateSurface(const RhiDescriptor& desc)
{
#ifdef PHX_PLATFORM_WINDOWS
    if (!desc.WindowsHandle)
    {
        PHX_CORE_ERROR("[RHI] WindowsHandle is null in RhiDescriptor.");
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.hwnd = static_cast<HWND>(desc.WindowsHandle);
    surface_create_info.hinstance = g_hInstance;

    VkResult result = vkCreateWin32SurfaceKHR(VkContext::vk_instance, &surface_create_info, GetVkAllocationCallbacks(), &VkContext::vk_surface);
    if (result != VK_SUCCESS)
    {
        PHX_CORE_ERROR("[RHI] Failed to create Win32 surface. VkResult: <TODO>");
        return false;
    }
    return true;
#else
    PHX_CORE_ERROR("[RHI] Platform not supported for surface creation yet.");
    return false;
#endif
}