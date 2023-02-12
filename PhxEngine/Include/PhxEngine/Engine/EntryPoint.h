#pragma once

#include <PhxEngine/Engine/EngineApp.h>
#include <PhxEngine/Engine/EngineCore.h>
#include <memory>
//#define USE_GLFW

// Client Code
extern PhxEngine::EngineApp* PhxEngine::CreateApplication(int argc, char* argv[]);
bool gApplicationRunning = true;


int main(int argc, char** argv)
{
    PhxEngine::CoreInitialize();
    {
        auto engineApp = std::unique_ptr<PhxEngine::EngineApp>(PhxEngine::CreateApplication(argc, argv));
        engineApp->Run();
    }
    PhxEngine::CoreFinalize();

    return 0;
}