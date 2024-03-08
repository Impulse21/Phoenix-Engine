#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/Core/Memory.h>

#include <RHI/D3D12/D3D12GfxDevice.h>

using namespace PhxEngine::RHI;

GfxDevice* GfxDeviceFactory::Create(RHI::GraphicsAPI preferedAPI)
{

	switch (preferedAPI)
	{
	case RHI::GraphicsAPI::DX12:
		PHX_LOG_CORE_ERROR("Creating DirectX 12 Graphics Device");
		return phx_new D3D12::D3D12GfxDevice;

	default:
		return nullptr;
	}
}