#include "pch.h"

#include "phxVulkanCore.h"
#include "phxVulkanDevice.h"

#include "vulkan/vulkan_win32.h"
#include <set>
#include <map>

using namespace phx::gfx::platform;


namespace phx::EngineCore
{
    extern HWND g_hWnd;
    extern HINSTANCE g_hInstance;
}

namespace
{
    const std::vector<const char*> kRequiredExtensions = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    // Optional extensions can also be done in the same way
    const std::vector<const char*> kOptionalExtensions =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    const std::vector<const char*> kValidationLayerPriorityList[] =
    {
        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
        {"VK_LAYER_KHRONOS_validation"},

        // Otherwise we fallback to using the LunarG meta layer
        {"VK_LAYER_LUNARG_standard_validation"},

        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects",
        },

        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
        {"VK_LAYER_LUNARG_core_validation"}
    };

    const std::vector<const char*> kRequiredDeviceExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    {
        PHX_CORE_WARN("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    {
        PHX_CORE_ERROR("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    {
        PHX_CORE_INFO("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    };

    return VK_FALSE;
}

void phx::gfx::platform::VulkanGpuDevice::Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle)
{
    m_enableValidationLayers = enableValidationLayers;
    m_extManager.Initialize();
    if (m_enableValidationLayers)
        m_layerManager.Initialize();

    CreateInstance();
    if (m_enableValidationLayers)
        SetupDebugMessenger();

    CreateSurface(windowHandle);
    PickPhysicalDevice();

    CreateLogicalDevice();
}

void phx::gfx::platform::VulkanGpuDevice::Finalize()
{
    vkDestroyDevice(m_vkDevice, nullptr);

    if (m_enableValidationLayers) 
    {
        DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
}

void phx::gfx::platform::VulkanGpuDevice::CreateInstance()
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

    std::vector<const char*> instanceExtensions = m_extManager.GetOptionalExtensions(kOptionalExtensions);
    
    m_extManager.CheckRequiredExtensions(kRequiredExtensions);

    for (const char* extName : kRequiredExtensions)
    {
        instanceExtensions.push_back(extName);
    }

    for (const char* extName : instanceExtensions)
    {
        PHX_CORE_INFO("[Vulkan] - Enabling Vulkan Extension '{0}'", extName);
    }

    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    createInfo.enabledLayerCount = 0;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> enabledLayers;
    for (auto& availableLayers : kValidationLayerPriorityList)
    {
        enabledLayers = m_layerManager.GetOptionalLayers(availableLayers);
        if (!enabledLayers.empty())
        {
            createInfo.enabledLayerCount = enabledLayers.size();
            createInfo.ppEnabledLayerNames = enabledLayers.data();
            break;
        }
    }

    for (const char* layerName : enabledLayers)
    {
        PHX_CORE_INFO("[Vulkan] - Enabling Vulkan Layer '{0}'", layerName);
    }


    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");
}

void phx::gfx::platform::VulkanGpuDevice::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;
    createInfo.pUserData = nullptr; // Optional

    VkResult result = CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug messenger!");
}

void phx::gfx::platform::VulkanGpuDevice::PickPhysicalDevice()
{
    m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

    PHX_CORE_INFO("[Vulkan] {0} Physical Devices found", deviceCount);
    if (deviceCount == 0) 
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int32_t, VkPhysicalDevice> candidates;

	for (const auto& device : devices)
	{
        int32_t score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
        m_vkPhysicalDevice = candidates.rbegin()->second;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProperties);
        PHX_CORE_INFO(
            "[Vulkan] Suitable device found {0} - Score:{1}",
            deviceProperties.deviceName,
            candidates.rbegin()->first);
	}
	else
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

    m_extManager.ObtainDeviceExtensions(m_vkPhysicalDevice);
}

void phx::gfx::platform::VulkanGpuDevice::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = 
    {
        indices.GraphicsFamily.value(),
        indices.ComputeFamily.value(),
        indices.TransferFamily.value(),
        indices.PresentFamily.value(),
    };


    const float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) 
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.enabledExtensionCount = kRequiredDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    // Modern Vulkan doesn't require this.
    createInfo.enabledLayerCount = 0;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
    if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }

    // Query queues
    vkGetDeviceQueue(m_vkDevice, indices.GraphicsFamily.value(), 0, &m_vkQueueGfx);
    vkGetDeviceQueue(m_vkDevice, indices.ComputeFamily.value(), 0, &m_vkComputeQueue);
    vkGetDeviceQueue(m_vkDevice, indices.TransferFamily.value(), 0, &m_vkTransferQueue);
    vkGetDeviceQueue(m_vkDevice, indices.PresentFamily.value(), 0, &m_vkPresentQueue);
}

void phx::gfx::platform::VulkanGpuDevice::CreateSurface(void* windowHandle)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = static_cast<HWND>(windowHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult result = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateSwapchain(SwapChainDesc const& desc)
{
    SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(m_vkPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = {};
    surfaceFormat.format = FormatToVkFormat(desc.Format);
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    bool valid = false;
    for (const auto& format : swapChainSupport.Formats)
    {
        if (!desc.EnableHDR && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            continue;

        if (format.format == surfaceFormat.format)
        {
            surfaceFormat = format;
            valid = true;
            break;
        }
    }

    if (!valid)
    {
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }

    m_swapChain.SwapChainImageFormat = surfaceFormat.format;

    if (swapChainSupport.Capabilities.currentExtent.width != 0xFFFFFFFF && swapChainSupport.Capabilities.currentExtent.height != 0xFFFFFFFF)
    {
        m_swapChain.SwapChainExtent = swapChainSupport.Capabilities.currentExtent;
    }
    else
    {
        m_swapChain.SwapChainExtent = { desc.Width, desc.Height };
        m_swapChain.SwapChainExtent.width = std::max(swapChainSupport.Capabilities.minImageExtent.width, std::min(swapChainSupport.Capabilities.maxImageExtent.width, m_swapChain.SwapChainExtent.width));
        m_swapChain.SwapChainExtent.height = std::max(swapChainSupport.Capabilities.minImageExtent.height, std::min(swapChainSupport.Capabilities.maxImageExtent.height, m_swapChain.SwapChainExtent.height));
    }

    uint32_t imageCount = std::max(static_cast<uint32_t>(kBufferCount), swapChainSupport.Capabilities.minImageCount);
    if ((swapChainSupport.Capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.Capabilities.maxImageCount))
    {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vkSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChain.SwapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
    if (!desc.VSync)
    {
        // The mailbox/immediate present mode is not necessarily supported:
        for (auto& presentMode : swapChainSupport.PresentModes)
        {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }


    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_swapChain.SwapChain;

    VkResult res = vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_swapChain.SwapChain);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to vreate swapchain");

    // Create Images for back buffers;
}

int32_t phx::gfx::platform::VulkanGpuDevice::RateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
    {
        return 0;
    }

    m_extManager.ObtainDeviceExtensions(device);

    for (auto req : kRequiredDeviceExtensions)
    {
        if (!m_extManager.IsDeviceExtensionAvailable(req))
        {
            return score = 0;
        }
    }

    QueueFamilyIndices indices = FindQueueFamilies(device);

    if (!indices.IsComplete())
    {
        return 0;
    }

    bool swapChainAdequate = false;

    SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(device);
    if (swapChainSupport.Formats.empty() || swapChainSupport.PresentModes.empty())
    {
        return 0;
    }

    return score;
}

QueueFamilyIndices phx::gfx::platform::VulkanGpuDevice::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.ComputeFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            indices.TransferFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &presentSupport);
        if (presentSupport)
        {
            indices.PresentFamily = i;
        }

        if (indices.IsComplete())
            return indices;

        i++;
    }

    return indices;
}

SwapChainSupportDetails phx::gfx::platform::VulkanGpuDevice::QuerySwapchainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &details.Capabilities);
   
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

    if (formatCount != 0) 
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0) 
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}
