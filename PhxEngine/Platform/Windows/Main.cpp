#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/Engine.h>

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

    std::unique_ptr<phx::IApplication> app(phx::CreateApplication());
    
    phx::Engine::Initialize(app.get());
    phx::Engine::Run();
    phx::Engine::Shutdown();

    app.reset();

    return 0;
}