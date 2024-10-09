#pragma once

#include "phxSpan.h"
namespace phx::gfx::platform
{
	namespace VulkanExtManager
	{
		void Initialize(bool enableValidation);
		void CheckRequiredExtensions(Span<const char*> requiredExtensions);
		bool IsExtensionAvailable(const char* extensionName);

	}
}