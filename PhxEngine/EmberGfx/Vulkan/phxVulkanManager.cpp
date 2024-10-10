#include "pch.h"
#include "phxVulkanManager.h"

namespace
{
}

void phx::gfx::platform::VulkanExtManager::Initialize()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    m_availableExtensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_availableExtensions.data());

    PHX_CORE_INFO("[Vulkan] - Available Vulkan extensions:");

    for (const auto& ext : m_availableExtensions) 
    {
        PHX_CORE_INFO("\t'{0}' Version:{1}", ext.extensionName, ext.specVersion);
        
    }
}

void phx::gfx::platform::VulkanExtManager::CheckRequiredExtensions(std::vector<const char*> const& requiredExtensions)
{
    for (const auto& reqExt : requiredExtensions) 
    {
        if (!IsExtensionAvailable(reqExt))
        {
            throw std::runtime_error(std::string("Required extension not available: ") + reqExt);
        }
    }
}

std::vector<const char*> phx::gfx::platform::VulkanExtManager::GetOptionalExtensions(std::vector<const char*> const& optionalExtensions)
{
    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(optionalExtensions.size());
    for (const auto& optExt : optionalExtensions)
    {
        if (IsExtensionAvailable(optExt)) 
        {
            enabledExtensions.push_back(optExt);
        }
    }

    return enabledExtensions;
}

bool phx::gfx::platform::VulkanExtManager::IsExtensionAvailable(const char* extensionName)
{
    for (const auto& ext : m_availableExtensions) 
    {
        if (strcmp(ext.extensionName, extensionName) == 0) 
        {
            return true;
        }
    }
    return false;
}

void phx::gfx::platform::VulkanExtManager::ObtainDeviceExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    m_availableDeviceExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, m_availableDeviceExtensions.data());
}

bool phx::gfx::platform::VulkanExtManager::IsDeviceExtensionAvailable(const char* extensionName)
{
    for (const auto& ext : m_availableDeviceExtensions)
    {
        if (strcmp(ext.extensionName, extensionName) == 0)
        {
            return true;
        }
    }
    return false;
}

void phx::gfx::platform::VulkanExtManager::LogDeviceExtensions()
{
    PHX_CORE_INFO("[Vulkan] - Available Vulkan Device extensions:");

    for (const auto& ext : m_availableDeviceExtensions)
    {
        PHX_CORE_INFO("\t'{0}' Version:{1}", ext.extensionName, ext.specVersion);
    }
}

void phx::gfx::platform::VulkanLayerManager::Initialize()
{
    uint32_t proeprtyCount = 0;
    vkEnumerateInstanceLayerProperties(&proeprtyCount, nullptr);

    m_availableLayers.resize(proeprtyCount);
    vkEnumerateInstanceLayerProperties(&proeprtyCount, m_availableLayers.data());

    PHX_CORE_INFO("[Vulkan] - Available Vulkan Layers:");

    for (const auto& layer : m_availableLayers)
    {
        PHX_CORE_INFO("\t'{0}' Version:{1}, ImplVersion:{2}, Description:{3}",
            layer.layerName,
            layer.specVersion,
            layer.implementationVersion,
            layer.description);
    }
}

void phx::gfx::platform::VulkanLayerManager::CheckRequiredLayers(std::vector<const char*> const& requiredlayers)
{
    for (const auto& req : requiredlayers)
    {
        if (!IsLayerAvailable(req))
        {
            throw std::runtime_error(std::string("Required layer not available: ") + req);
        }
    }
}

std::vector<const char*> phx::gfx::platform::VulkanLayerManager::GetOptionalLayers(std::vector<const char*> const& optionalLayers)
{
    std::vector<const char*> enabledLayers;
    enabledLayers.reserve(optionalLayers.size());
    for (const auto& opt : optionalLayers)
    {
        if (IsLayerAvailable(opt))
        {
            enabledLayers.push_back(opt);
        }
    }

    return enabledLayers;
}

bool phx::gfx::platform::VulkanLayerManager::IsLayerAvailable(const char* layerName)
{
    for (const auto& layer : m_availableLayers)
    {
        if (strcmp(layer.layerName, layerName) == 0)
        {
            return true;
        }
    }

    return false;
}
