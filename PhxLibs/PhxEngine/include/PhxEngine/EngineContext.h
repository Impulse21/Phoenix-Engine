#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Platform/PlatformWindow.h>

// -- Forward Declares ---
namespace phx
{
    class IIoQueue;
    class IVirtualFileSystem;
}

namespace phx
{
    struct EngineContext
    {
        // -- Depericated ---
        IVirtualFileSystem* virtual_file_system;
        
        // -- Depericated ---
        IIoQueue* io_queue;

        WindowHandle window_handle;
    };
}