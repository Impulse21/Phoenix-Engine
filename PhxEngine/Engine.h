#pragma once

#include <PhxEngine/Memory/Memory.h>

namespace phx
{
    class IApplication;
    
    struct EngineDesc
    {
        const char* app_name;
        
        u32                 window_width;
        u32                 window_height;

        bool                headless        = true;
        Memory::Desc        memory_desc = {};
    };
    
    namespace Engine
    {
        void Initialize(IApplication* app);
        void Run();
        void Shutdown();

        void RequestExit();
    }
}