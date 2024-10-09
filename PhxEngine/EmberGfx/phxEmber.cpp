#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"
#include "phxCommandLineArgs.h"



// Vulkan
#include "Vulkan/phxVulkanDevice.h"

void phx::gfx::InitializeWindows(
	SwapChainDesc const& swapChainDesc,
	void* windowHandle)
{
	uint32_t useValidation = 0;
#if _DEBUG
	// Default to true for debug builds
	useValidation = 1;
#endif
	CommandLineArgs::GetInteger(L"debug", useValidation);

	platform::VulkanDevice::Initialize(swapChainDesc, (bool)useValidation, windowHandle);
	GfxDevice::Initialize(swapChainDesc, windowHandle);


}

void phx::gfx::Finalize()
{
	platform::VulkanDevice::Finalize();
	GfxDevice::Finalize();
}