#pragma once

#include <PhxEngine/Core/Handle.h>

namespace phx::platform
{
    struct OSWindow;
    using OSWindowHandle = Handle<OSWindow>;

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
    
    OSWindowHandle CreateOSWindow(const WindowDescriptor& desc);
    void DestroyOSWindow(OSWindowHandle handle);
    void PollEvents();
    bool ShouldClose(OSWindowHandle handle);
    void* GetNativeHandle(OSWindowHandle handle); 
}