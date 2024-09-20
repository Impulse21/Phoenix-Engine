#include "pch.h"
#include "phxGfxCore.h"

#if defined(PHX_PLATFORM_WINDOWS)
#include "D3D12/phxGfxDevice.h"
#include "D3D12/phxGfxGpuMemoryManager.h"
#include "D3D12/phxGfxResourceManager.h"
#include "D3D12/phxGfxRenderer.h"


#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

#endif

void phx::gfx::InitializeNull()
{
}

#if defined(PHX_PLATFORM_WINDOWS)
void phx::gfx::InitializeWindows(SwapChainDesc const& desc, HWND hWnd)
{
	Device::Ptr = new D3D12Device(desc, hWnd);
	GpuMemoryManager::Ptr = new D3D12GpuMemoryManager(D3D12Device::Impl());
	ResourceManager::Ptr = new D3D12ResourceManager();
	Renderer::Ptr = new D3D12Renderer();
}
#endif

void phx::gfx::Finalize()
{
	delete Renderer::Ptr;
	Renderer::Ptr = nullptr;

	delete ResourceManager::Ptr;
	ResourceManager::Ptr = nullptr;

	delete Device::Ptr;
	Device::Ptr = nullptr;
}
