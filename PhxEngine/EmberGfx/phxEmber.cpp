#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"
#include "phxCommandLineArgs.h"


// Vulkan
#include "Vulkan/phxVulkanExtManager.h"

void phx::gfx::InitializeWindows(
	SwapChainDesc const& swapChainDesc,
	void* windowHandle)
{
	Ember::Ptr = new Ember();

	uint32_t useValidation = 0;
#if _DEBUG
	// Default to true for debug builds
	useValidation = 1;
#endif
	CommandLineArgs::GetInteger(L"debug", useValidation);

	platform::VulkanExtManager::Initialize((bool)useValidation);
	GfxDevice::Initialize(swapChainDesc, windowHandle);
}

void phx::gfx::Finalize()
{
	delete Ember::Ptr;
	Ember::Ptr = nullptr;
}


phx::gfx::Ember::Ember() = default;

phx::gfx::Ember::~Ember()
{
	GfxDevice::Finalize();
}
