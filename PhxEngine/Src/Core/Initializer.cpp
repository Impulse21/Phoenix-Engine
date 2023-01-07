#include "phxpch.h"
#include "PhxEngine/Core/Initializer.h"
#include <PhxEngine/Core/Log.h>
#include "PhxEngine/RHI/PhxRHI.h"

using namespace PhxEngine::RHI;

void PhxEngine::Core::Initialize()
{
	// Create Graphics Core
	Log::Initialize();
	RHI::IGraphicsDevice::Ptr = RHI::DeviceFactory::CreateDx12Device();
}

void PhxEngine::Core::Finalize()
{
	if (RHI::IGraphicsDevice::Ptr)
	{
		delete IGraphicsDevice::Ptr;
		IGraphicsDevice::Ptr = nullptr;
	};

	RHI::ReportLiveObjects();
}