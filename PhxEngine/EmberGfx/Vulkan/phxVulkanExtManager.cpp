#include "pch.h"
#include "phxVulkanExtManager.h"

namespace
{
	bool m_validationLayersEnabled;
	std::vector<VkExtensionProperties> m_availableExtensions;
}

void phx::gfx::platform::VulkanExtManager::Initialize(bool enableValidation)
{
	m_validationLayersEnabled = enableValidation;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    m_availableExtensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_availableExtensions.data());

    PHX_CORE_INFO("[Vulkan] - Available Vulkan extensions:");

    for (const auto& ext : m_availableExtensions) 
    {
        PHX_CORE_INFO("\t- '{0}' Version:{1}", ext.extensionName, ext.specVersion);
        
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
