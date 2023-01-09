#include "phxpch.h"
#include "PhxEngine/Core/Initializer.h"
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine::RHI;

void PhxEngine::Core::Initialize()
{
	// Create Graphics Core
	Log::Initialize();
	
	RHIInitialize();
}

void PhxEngine::Core::Finalize()
{
	RHIFinalize();

	RHI::ReportLiveObjects();
}