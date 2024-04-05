#pragma once

#include <PhxEngine/Core/Application.h>


extern PhxEngine::Application* PhxEngine::CreateApplication(ApplicationCommandLineArgs args);


int main(int argc, char** argv)
{
	// TODO:: Add Engine Core initialization
	auto app = PhxEngine::CreateApplication({ argc, argv });
	app->Run();
	delete app;
}