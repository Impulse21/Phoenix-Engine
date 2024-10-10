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

	platform::VulkanGpuDevice gpuDevice;

	gpuDevice.Initialize(swapChainDesc, (bool)useValidation, windowHandle);
	GfxDevice::Initialize(swapChainDesc, windowHandle);

	gpuDevice.Finalize();
}

void phx::gfx::Finalize()
{
	GfxDevice::Finalize();
}