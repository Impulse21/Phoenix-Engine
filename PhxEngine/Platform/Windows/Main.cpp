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

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    PHX_UNUSED(hInstance);
    PHX_UNUSED(hPrevInstance);
    PHX_UNUSED(lpCmdLine);
    PHX_UNUSED(nCmdShow);

#if defined(PHX_DEBUG)
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin,  "CONIN$",  "r", stdin);
#endif

    phx::IApplication* app = phx::CreateApplication();
    
    phx::Engine::Initialize(app, {});
    phx::Engine::Run();
    phx::Engine::Shutdown();

    delete app;
#if defined(PHX_DEBUG)
    std::cout << "\n[PhxEngine] Shutdown complete. Press any key to exit..." << std::endl;
    _getch();
    FreeConsole();
#endif

    return 0;
}