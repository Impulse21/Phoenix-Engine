#pragma once

#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/App/Application.h>

#include <PhxEngine/RHI/PhxRHI_Dx12.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::New;

int main(int argc, char** argv)
{
	// -- Core System Initializtion ---
	PhxEngine::LogSystem::Initialize();

	ApplicationCommandLineArgs args = { argc, argv };

	// Creating Device
	LOG_CORE_INFO("Creating DX12 Graphics Device");
	std::unique_ptr<IGraphicsDevice> graphicsDevice = Dx12::Factory::CreateDevice();

	{
		auto app = CreateApplication(args, graphicsDevice.get());

		app->Run();
	}
	graphicsDevice.reset();

	ReportLiveObjects();
}