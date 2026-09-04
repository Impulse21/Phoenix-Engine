#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/Engine.h>


#if defined(PHX_DEBUG)
    #include <iostream>
    #include <conio.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


// Force the use of the high-performance NVIDIA GPU
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

// Optional: Force high-performance AMD card on dual systems
extern "C" {
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main(int argc, char* argv[])
{
    phx::IApplication* app = phx::CreateApplication();
    
    phx::Engine::Initialize(app, phx::Span<char*>(argv, argc));
    phx::Engine::Run();
    phx::Engine::Shutdown();

    delete app;

    return 0;
}