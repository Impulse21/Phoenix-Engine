#include <PhxEngine/IApplication.h>
#include <PhxEngine/Engine.h>

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/// @brief Forward declaration of the application entry point. The user must implement this function in their application code. 
namespace phx 
{ 
    IApplication* CreateApplication();
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    std::unique_ptr<phx::IApplication> app(phx::CreateApplication());
    
    phx::Engine::Initialize(*app);
    phx::Engine::Run();
    phx::Engine::Shutdown();

    app.reset();

    return 0;
}