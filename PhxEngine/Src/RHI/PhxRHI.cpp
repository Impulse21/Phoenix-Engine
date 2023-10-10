#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12/D3D12GfxDevice.h"
#include <PhxEngine/Core/Memory.h>
using namespace PhxEngine;

namespace
{
	std::unique_ptr<RHI::GfxDevice> CreateDevice_Windows(RHI::GraphicsAPI api)
	{
		switch (api)
		{
		case RHI::GraphicsAPI::DX12:
			LOG_RHI_INFO("Creating DirectX 12 Graphics Device");
			return std::make_unique<RHI::D3D12::D3D12GfxDevice>();

		default:
			return nullptr;
		}
	}
}

std::unique_ptr<RHI::GfxDevice> PhxEngine::RHI::Factory::CreateD3D12Device()
{
	return CreateDevice_Windows(RHI::GraphicsAPI::DX12);
}
