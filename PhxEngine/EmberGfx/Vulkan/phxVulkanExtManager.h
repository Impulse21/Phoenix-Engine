#pragma once

#include "phxSpan.h"
namespace phx::gfx::platform
{
	namespace VulkanExtManager
	{
		void Initialize(bool enableValidation);
		void CheckRequiredExtensions(std::vector<const char*> const& requiredExtensions);
		std::vector<const char*> GetOptionalExtensions(std::vector<const char*> const& optionalExtensions);
		bool IsExtensionAvailable(const char* extensionName);

	}
}