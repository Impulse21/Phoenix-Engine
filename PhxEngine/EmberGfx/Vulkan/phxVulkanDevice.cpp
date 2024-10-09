#include "pch.h"
#include "phxVulkanDevice.h"

#include "phxVulkanExtManager.h"
#include "vulkan/vulkan_win32.h"


namespace
{
    static const std::vector<const char*> kRequiredExtensions = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    // Optional extensions can also be done in the same way
    static const std::vector<const char*> kOptionalExtensions =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
}

void phx::gfx::platform::VulkanDevice::Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle)
{
	VulkanExtManager::Initialize(enableValidationLayers);
    CreateInstance();
}

void phx::gfx::platform::VulkanDevice::Finalize()
{
    vkDestroyInstance(m_vkInstance, nullptr);
}

void phx::gfx::platform::VulkanDevice::CreateInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "PHX Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;


    std::vector<const char*> instanceLayers;
    std::vector<const char*> instanceExtensions = VulkanExtManager::GetOptionalExtensions(kOptionalExtensions);
    
    VulkanExtManager::CheckRequiredExtensions(kRequiredExtensions);

    for (const char* extName : kRequiredExtensions)
    {
        instanceExtensions.push_back(extName);
    }

    for (const char* extName : instanceExtensions)
    {
        PHX_CORE_INFO("[Vulkan] - Enabling Vulkan Extension '{0}'", extName);
    }

    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
}
