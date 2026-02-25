#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Result.h>
#include <PhxCore/Platform/PlatformConfig.h>

namespace phx
{

    struct Window;
    using WindowHandle = Handle<Window>;

    struct WindowDescriptor
    {
        uint32_t width;
        uint32_t height;
        const char* title;

        union
        {
            uint32_t FlagBits;
            struct
            {
                uint32_t FullScreen : 1;
                uint32_t VSync      : 1;
            } flags;   
        };
    };
    
    namespace Platform
    {    
        Result<WindowHandle> CreateWindow(const WindowDescriptor& desc);
        void DestoryWindow(WindowHandle handle);
        bool PollEvents(WindowHandle handle);
        window_native_handle GetNativeHandle(WindowHandle handle); 
    }
}
