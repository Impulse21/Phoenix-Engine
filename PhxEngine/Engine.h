#pragma once

#include <PhxEngine/Memory/Memory.h>

namespace phx
{
    class IApplication;
    
    struct EngineDesc
    {
        const char* app_name;
        
        uint32_t    window_width;
        uint32_t    window_height;

        bool        headless        = true;
    };
    
    namespace Engine
    {
        void Initialize(IApplication* app);
        void Run();
        void Shutdown();

        void RequestExit();
    }
}