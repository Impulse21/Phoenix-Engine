#include <stdio.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/Engine.h>

#include <memory>

/// @brief Forward declaration of the application entry point. The user must implement this function in their application code. 
namespace phx 
{ 
    IApplication* CreateApplication();
}

int main(int argc, char* argv[], char* envp[]) 
{   
    PHX_UNUSED(argc);
    PHX_UNUSED(argv);
    PHX_UNUSED(envp);

    std::unique_ptr<phx::IApplication> app(phx::CreateApplication());
    
    phx::Engine::Initialize(app.get());
    phx::Engine::Run();
    phx::Engine::Shutdown();

    app.reset();

    return 0;
}