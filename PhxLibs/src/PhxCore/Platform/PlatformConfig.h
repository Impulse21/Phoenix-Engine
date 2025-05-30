#pragma once

#ifdef PHX_PLATFORM_WINDOWS

#include <PhxCore/windows/WindowsPlatformWrapper.h>

    namespace phx::platform 
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        using PlatformWrapper = windows::WindowsPlatformWrapperImpl;
    }
    
#else

#error "Unsupported platform. Currently only support windows."

#endif