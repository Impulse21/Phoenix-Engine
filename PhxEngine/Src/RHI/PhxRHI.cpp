#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12/D3D12GfxDevice.h"
#include <PhxEngine/Core/Memory.h>
using namespace PhxEngine;

namespace
{
	std::unique_ptr<RHI::GfxDevice> gSingleton;

	std::unique_ptr<RHI::GfxDevice> CreateDevice_Windows(RHI::GraphicsAPI api)
	{
		switch (api)
		{
		case RHI::GraphicsAPI::DX12:
			return std::make_unique<RHI::D3D12::D3D12GfxDevice>();

		default:
			return nullptr;
		}
	}
}

void PhxEngine::RHI::Setup::Initialize(RhiParameters const& parameters)
{
	gSingleton = CreateDevice_Windows(parameters.Api);
	gSingleton->Initialize(parameters.SwapChainDesc, parameters.WindowHandle);
}

void PhxEngine::RHI::Setup::Finalize()
{
	gSingleton->WaitForIdle();
	gSingleton->Finalize();
	gSingleton.reset();
}

RHI::GfxDevice* PhxEngine::RHI::GetGfxDevice()
{
	return gSingleton.get();
}
