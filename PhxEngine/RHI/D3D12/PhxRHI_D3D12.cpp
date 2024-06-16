#include "pch.h"
#include "phxRHI_d3d12.h"

using namespace phx;
using namespace phx::rhi;
using namespace Microsoft::WRL;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
    // Used to enable the "Agility SDK" components
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif


namespace
{
    const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
    const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	// helper function for texture subresource calculations
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dn705766(v=vs.85).aspx
	uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize)
	{
		return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
	}

}

phx::rhi::D3D12GfxDevice::D3D12GfxDevice(Config const& config)
	: m_resourceManager(this)
{
	D3D12GfxDevice::Ptr = this;

	this->InitializeDeviceResources(config);
}

phx::rhi::D3D12GfxDevice::~D3D12GfxDevice()
{
	D3D12GfxDevice::Ptr = nullptr;
}

void phx::rhi::D3D12GfxDevice::InitializeDeviceResources(Config const& config)
{
}
