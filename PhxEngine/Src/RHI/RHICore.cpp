#include <PhxEngine/RHI/RHICore.h>

#include <RHI/GfxDriver.h>
#include <memory>

#include "D3D12/D3D12Driver.h"
#include <assert.h>

using namespace PhxEngine::RHI;

namespace
{
	std::unique_ptr<GfxDriver> _DriverInstnace;
}

bool PhxEngine::RHI::Initialize(RHIParmaeters const& parameters)
{
	assert(!_DriverInstnace);

	// Create Driver
	switch (parameters.Api)
	{
	case GraphicsAPI::DX12:
		_DriverInstnace = std::make_unique<D3D12::D3D12Driver>();
		break;

	default:
		return true;
	}

	// Set up global driver for the RHI module.
	GfxDriver::Impl = _DriverInstnace.get();
	GfxDriver::Impl->Initialize();

	return true;
}

void PhxEngine::RHI::Finalize()
{
	assert(_DriverInstnace);

	// Wait for Idle
	// Shutdown
	GfxDriver::Impl->Finialize();
	GfxDriver::Impl = nullptr;
	_DriverInstnace.reset();
}

void PhxEngine::RHI::Present(const SwapChain& Swapchain)
{
}

void PhxEngine::RHI::RunGarbageCollection()
{
}

void PhxEngine::RHI::WaitForIdle()
{
}

