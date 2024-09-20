#include "pch.h"
#include "phxGfx.h"

#if defined(PHX_PLATFORM_WINDOWS)
#include "D3D12/phxGfxDevice.h"
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
void phx::gfx::InitializeWindows(HWND hWnd)
{
	Device::Ptr = new D3D12Device(hWnd);
	ResourceManager::Ptr = new D3D12ResourceManager();
	Renderer::Ptr = new D3D12Renderer();
}
#endif

void phx::gfx::Shutdown()
{
	delete Renderer::Ptr;
	Renderer::Ptr = nullptr;

	delete ResourceManager::Ptr;
	ResourceManager::Ptr = nullptr;

	delete Device::Ptr;
	Device::Ptr = nullptr;
}
