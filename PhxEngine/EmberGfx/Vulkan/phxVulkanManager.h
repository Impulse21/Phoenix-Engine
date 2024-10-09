#pragma once

#include "phxSpan.h"
namespace phx::gfx::platform
{
	class VulkanExtManager
	{
	public:
		void Initialize();
		void CheckRequiredExtensions(std::vector<const char*> const& requiredExtensions);
		std::vector<const char*> GetOptionalExtensions(std::vector<const char*> const& optionalExtensions);
		bool IsExtensionAvailable(const char* extensionName);

		void ObtainDeviceExtensions(VkPhysicalDevice device);
		bool IsDeviceExtensionAvailable(const char* extensionName);

		void LogDeviceExtensions();
	private:
		std::vector<VkExtensionProperties> m_availableExtensions;
		std::vector<VkExtensionProperties> m_availableDeviceExtensions;
	};

	class VulkanLayerManager
	{
	public:
		void Initialize();

		void CheckRequiredLayers(std::vector<const char*> const& requiredlayers);
		std::vector<const char*> GetOptionalLayers(std::vector<const char*> const& optionalLayers);
		bool IsLayerAvailable(const char* layerName);

	private:
		std::vector<VkLayerProperties> m_availableLayers;
	};
}