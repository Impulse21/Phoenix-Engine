#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>

#include <RHI/D3D12/D3D12GfxDevice.h>

using namespace PhxEngine::RHI;

GfxDevice* GfxDeviceFactory::Create(RHI::GraphicsAPI preferedAPI)
{

	switch (preferedAPI)
	{
	case RHI::GraphicsAPI::DX12:
		LOG_RHI_INFO("Creating DirectX 12 Graphics Device");
		return phx_new(D3D12::D3D12GfxDevice);

	default:
		return nullptr;
	}
}