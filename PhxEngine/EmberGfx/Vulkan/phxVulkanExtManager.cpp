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
        PHX_CORE_INFO("\t[Extension] - {0} Version:{1}", ext.extensionName, ext.specVersion);
        
    }
}

void phx::gfx::platform::VulkanExtManager::CheckRequiredExtensions(Span<const char*> requiredExtensions)
{
}

bool phx::gfx::platform::VulkanExtManager::IsExtensionAvailable(const char* extensionName)
{
    return false;
}
