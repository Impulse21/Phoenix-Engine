#include "pch.h"
#include "phxCommandLineArgs.h"

#include "phxGfxPlatformImpl.h"

#include "phxGfxHandlePool.h"

using namespace phx::gfx;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

#define SCOPE_LOCK(mutex) std::scoped_lock _(mutex)

namespace
{
	static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	phx::gfx::HandlePool<platform::D3D12SwapChain, PlatformSwapChain> poolSwapChain;
}

namespace
{
	HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter)
	{
		return factory6->EnumAdapterByGpuPreference(
			adapterIndex,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(outAdapter));
	}

	void InitializeD3D12Context()
	{
		uint32_t useDebugLayers = 0;
#if _DEBUG
		// Default to true for debug builds
		useDebugLayers = 1;
#endif
		CommandLineArgs::GetInteger(L"debug", useDebugLayers);
	}
}

namespace phx::gfx
{
	namespace platform
	{
		ID3D12Device* g_Device;

		void Initialize()
		{
			poolSwapChain.Initialize(1);

			InitializeD3D12Context();
		
		}

		void Finalize()
		{
			platform::IdleGpu();

			poolSwapChain.Finalize();
		}
	}
}
