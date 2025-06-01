#pragma once

#ifdef PHX_PLATFORM_WINDOWS
#include "windows\WindowsPlatformWrapper.h"

    namespace phx::platform 
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        using PlatformWrapper = windows::WindowsPlatformWrapperImpl;
        using window_type = HWND;
    }
    
#else

#error "Unsupported platform. Currently only support windows."

#endif