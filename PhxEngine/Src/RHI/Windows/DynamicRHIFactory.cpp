#include <RHI/DynamicRHIFactory.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/RHI/PhxDynamicRHI.h>

#include <RHI/D3D12/D3D12DynamicRHI.h>

using namespace PhxEngine::RHI;


DynamicRHI* DynamicRHIFactory::Create(RHI::GraphicsAPI preferedAPI)
{

	switch (preferedAPI)
	{
	case RHI::GraphicsAPI::DX12:
		LOG_RHI_INFO("Creating DirectX 12 Graphics Device");
		return phx_new(D3D12::D3D12DynamicRHI);

	default:
		return nullptr;
	}
}