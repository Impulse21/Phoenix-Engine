#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12/D3D12GfxDevice.h"
#include <PhxEngine/Core/Memory.h>
using namespace PhxEngine;

namespace
{
	std::unique_ptr<RHI::GfxDevice> gSingleton;
	std::shared_ptr<Core::HeapAllocator> gResourceMemoryAllocator;

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
	gResourceMemoryAllocator = std::make_shared<Core::HeapAllocator>();
	gResourceMemoryAllocator->Initialize(parameters.DynamicMemoryPoolSize);

	gSingleton = CreateDevice_Windows(parameters.Api);
	gSingleton->Initialize(gResourceMemoryAllocator, parameters.SwapChainDesc, parameters.WindowHandle);
}

void PhxEngine::RHI::Setup::Finalize()
{
	gSingleton->WaitForIdle();
	gSingleton->Finalize();
	gSingleton.reset();

	gResourceMemoryAllocator->Finalize();
	gResourceMemoryAllocator.reset();
}

RHI::GfxDevice* PhxEngine::RHI::GetGfxDevice()
{
	return gSingleton.get();
}
