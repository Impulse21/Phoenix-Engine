#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Engine/EngineCore.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine::RHI;

void PhxEngine::CoreInitialize()
{
	// Create Graphics Core
	Core::Log::Initialize();
	
	RHIInitialize();
}

void PhxEngine::CoreFinalize()
{
	RHIFinalize();

	RHI::ReportLiveObjects();
}