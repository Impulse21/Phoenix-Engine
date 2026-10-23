#include <stdio.h>
#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/Runtime.h>


#if defined(PHX_DEBUG)
    #include <iostream>
#endif

int main(int argc, char* argv[], char* envp[]) 
{   
    PHX_UNUSED(envp);

    phx::IApplication* app = phx::CreateApplication();
    
    phx::Runtime::Initialize(app, phx::Span<char*>(argv, argc));
    phx::Runtime::Run();
    phx::Runtime::Shutdown();

    delete app;

#if defined(PHX_DEBUG)
    std::cout << "\n[PhxEngine] Shutdown complete. Press any key to exit..." << std::endl;
    std::cin.get();
#endif
    return 0;
}