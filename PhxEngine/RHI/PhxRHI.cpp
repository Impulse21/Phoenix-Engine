#include "pch.h"
#include "phxRHI.h"

#ifdef _WIN32
#include "D3D12/phxRHI_d3d12.h"
#endif

using namespace phx;
using namespace phx::rhi;

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace
{
#ifdef _WIN32
	void InitD3D12(Config const& config)
	{
		PHX_CORE_INFO("Init D3D12 RHI backend");
		GfxDevice::Ptr = new D3D12GfxDevice(config);
	}
#endif
}

#ifdef _WIN32
void rhi::Initialize_Windows(Config const& config)
{
	switch (config.Api)
	{
	case GraphicsAPI::DX12:
	default:
		InitD3D12(config);
	}
}
#endif

void rhi::Finalize()
{
	GfxDevice::Ptr->WaitForIdle();
	delete GfxDevice::Ptr;
}
