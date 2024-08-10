#include "pch.h"
#include "phxRHI.h"

#ifdef _WIN32
#include "D3D12/PhxRHI_D3D12.h"
#endif

using namespace phx;
using namespace phx::rhi;

extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace
{
#ifdef _WIN32
	void InitD3D12()
	{
		using namespace phx::rhi::d3d12;
		PHX_CORE_INFO("Init D3D12 RHI backend");
		GfxDevice::Ptr = new D3D12GfxDevice();
	}
#endif
}

#ifdef _WIN32
void rhi::Initialize_Windows(rhi::GraphicsAPI preferedAPI)
{
	switch (preferedAPI)
	{
	case GraphicsAPI::DX12:
	default:
		InitD3D12();
	}
}
#endif

void rhi::Finalize()
{
	if (!GfxDevice::Ptr)
		return;

	GfxDevice::Ptr->WaitForIdle();
	delete GfxDevice::Ptr;
}
