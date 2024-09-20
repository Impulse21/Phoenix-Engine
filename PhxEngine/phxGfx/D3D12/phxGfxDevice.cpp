#include "pch.h"
#include "phxGfxDevice.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2


phx::gfx::D3D12Device::D3D12Device(HWND hwnd)
{
	assert(Singleton == nullptr);
	Singleton = this;

	// Initialize TOOD:
}
