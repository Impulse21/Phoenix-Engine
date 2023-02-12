#include <phxpch.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include "RHI/GfxDeviceFactory.h"

using namespace PhxEngine::RHI;


std::unique_ptr<IGraphicsDevice> PhxEngine::RHI::CreatePlatformGfxDevice()
{
	LOG_CORE_INFO("Initializing RHI on Windows platform");

	LOG_CORE_INFO("Currently supports: D3D12. Future supports Vulkan, Null");

	// Get An abstract factory from the provider
	GfxDeviceFactoryProvider provider;
	std::unique_ptr<IGraphicsDeviceFactory> factory = provider.CreatGfxDeviceFactory(RHIType::D3D12);

	return factory->CreateDevice();
}