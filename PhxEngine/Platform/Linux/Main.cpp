#include <stdio.h>
#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/Engine.h>

#include <memory>

#if defined(PHX_DEBUG)
    #include <iostream>
#endif

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

#if defined(PHX_DEBUG)
    std::cout << "\n[PhxEngine] Shutdown complete. Press any key to exit..." << std::endl;
    std::cin.get();
#endif
    return 0;
}