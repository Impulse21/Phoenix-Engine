#include <phxpch.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include "RHI/RHIFactory.h"

using namespace PhxEngine::RHI;


std::unique_ptr<IRHI> PhxEngine::RHI::PlatformCreateRHI()
{
	LOG_CORE_INFO("Initializing RHI on Windows platform");

	LOG_CORE_INFO("Currently supports: D3D12. Future supports Vulkan, Null");

	// Get An abstract factory from the provider
	RHIFactoryProvider provider;
	std::unique_ptr<IRHIFactory> factory = provider.CreateRHIFactory(RHIType::D3D12);

	return factory->CreateRHI();
}