#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Result.h>
#include <PhxCore/Platform/PlatformConfig.h>

namespace phx
{
    struct WindowDescriptor
    {
        uint32_t width;
        uint32_t height;
        const char* title;

        union
        {
            uint32_t FlagBits;
            struct WindowFlags
            {
                uint32_t FullScreen : 1;
                uint32_t VSync      : 1;
            } flags;   
        };
    };

    PHX_DEFINE_OPAQUE_HANDLE(Window)
        
    namespace Platform
    {    
        Result<Window> CreateWindow(const WindowDescriptor& desc);
        void DestoryWindow(Window handle);
        bool PullEvents(Window handle);
        window_native_handle GetNativeHandle(Window handle); 
    }
}
